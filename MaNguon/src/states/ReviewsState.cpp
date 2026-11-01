#include "states/ReviewsState.hpp"
#include "core/Utf8.hpp"
#include "ui/Art.hpp"
#include <SFML/Window/Clipboard.hpp>
#include <algorithm>
#include <array>
#include <cmath>
#include <iomanip>
#include <sstream>

namespace tiem {
namespace {
constexpr std::size_t perPage = 2;
constexpr std::size_t replyLimit = 240;
const sf::FloatRect inputArea{220, 368, 840, 130};

std::string asUtf8(const sf::String &text) {
    const auto bytes = text.toUtf8();
    return {bytes.begin(), bytes.end()};
}

std::vector<sf::String> wrap(const sf::String &value, const sf::Font &font, unsigned size, float width) {
    std::vector<sf::String> lines;
    sf::String line;
    sf::Text measure("", font, size);
    for (const auto codepoint : value) {
        if (codepoint == '\n') {
            lines.push_back(line);
            line.clear();
            continue;
        }
        line += codepoint;
        measure.setString(line);
        if (measure.getLocalBounds().width <= width || line.getSize() < 2) continue;
        std::size_t cut = line.getSize() - 1;
        for (std::size_t n = line.getSize() - 1; n > 0; --n)
            if (line[n] == ' ') {
                cut = n;
                break;
            }
        lines.push_back(line.substring(0, cut));
        line = line.substring(cut);
        while (!line.isEmpty() && line[0] == ' ') line.erase(0, 1);
    }
    if (!line.isEmpty() || lines.empty()) lines.push_back(line);
    return lines;
}

void paragraph(sf::RenderTarget &target, const sf::Font &font, const sf::String &value,
               unsigned size, sf::Vector2f position, float width, float lineHeight,
               std::size_t maxLines, sf::Color color, std::size_t start = 0) {
    const auto lines = wrap(value, font, size, width);
    for (std::size_t i = start; i < lines.size() && i < start + maxLines; ++i) {
        auto line = lines[i];
        if (i == start + maxLines - 1 && i + 1 < lines.size()) line += utf8("…");
        ui::text(target, font, asUtf8(line), size,
                 {position.x, position.y + (i - start) * lineHeight}, color);
    }
}

void stars(sf::RenderTarget &target, int rating, sf::Vector2f position, float radius, float spacing) {
    for (int i = 0; i < 5; ++i)
        ui::star(target, {position.x + i * spacing, position.y}, radius,
                 i < rating ? sf::Color(227, 165, 54) : sf::Color(216, 207, 184));
}
bool blank(const sf::String &value) {
    for (auto c : value)
        if (c > 32 && c != 0x00a0 && c != 0x200b && c != 0x3000) return false;
    return true;
}
} // namespace

ReviewsState::ReviewsState(StateStack &stack, AppContext &context) : UiState(stack, context) {
    rebuildButtons();
}

std::vector<const ShopReview *> ReviewsState::filtered() const {
    std::vector<const ShopReview *> result;
    for (auto it = game().progress.reviews.rbegin(); it != game().progress.reviews.rend(); ++it)
        if (!unansweredOnly_ || it->reply.empty()) result.push_back(&*it);
    return result;
}

ShopReview *ReviewsState::editingReview() {
    if (!editing_) return nullptr;
    for (auto &review : game().progress.reviews)
        if (review.id == *editing_) return &review;
    return nullptr;
}

void ReviewsState::rebuildButtons() {
    buttons_.clear();
    rebuild_ = false;
    if (editing_) {
        addButton("Quay lại", {220, 586, 156, 43}, [this] { closeEditor(); });
        addButton("Lưu phản hồi", {853, 586, 207, 43}, [this] { submitReply(); });
        const std::array<std::pair<const char *, const char *>, 3> suggestions{{
            {"Cảm ơn khách", "Cảm ơn bạn đã ghé tiệm và chia sẻ cảm nhận! Tiệm rất vui vì bạn đã có một bữa ăn ngon. Hẹn gặp bạn lần tới nhé!"},
            {"Xin lỗi khách", "Tiệm xin lỗi vì trải nghiệm chưa như bạn mong đợi. Cảm ơn bạn đã góp ý thật lòng, tiệm sẽ chăm chút hơn cho từng phần bánh."},
            {"Ghi nhận góp ý", "Tiệm đã đọc kỹ góp ý của bạn rồi. Cảm ơn bạn đã dành thời gian chia sẻ, tiệm sẽ điều chỉnh để lần ghé sau vui hơn nhé!"}
        }};
        for (std::size_t i = 0; i < suggestions.size(); ++i) {
            const std::string reply = suggestions[i].second;
            addButton(suggestions[i].first, {220.f + i * 282.f, 526, 266, 36}, [this, reply] {
                draft_ = utf8(reply);
                inputFocused_ = true;
                selectAll_ = false;
                notice_.clear();
                blink_ = 0;
            });
        }
        return;
    }
    addButton("Đóng", {1056, 61, 145, 44}, [this] { stack_.pop(); });
    addButton(unansweredOnly_ ? "Tất cả" : "• Tất cả", {680, 110, 170, 33}, [this] {
        unansweredOnly_ = false;
        page_ = 0;
        rebuild_ = true;
    });
    addButton(unansweredOnly_ ? "• Chưa phản hồi" : "Chưa phản hồi", {864, 110, 337, 33}, [this] {
        unansweredOnly_ = true;
        page_ = 0;
        rebuild_ = true;
    });
    const auto items = filtered();
    const auto pages = std::max<std::size_t>(1, (items.size() + perPage - 1) / perPage);
    page_ = std::min(page_, pages - 1);
    for (std::size_t row = 0; row < perPage; ++row) {
        const auto index = page_ * perPage + row;
        if (index >= items.size()) break;
        const auto id = items[index]->id;
        addButton(items[index]->reply.empty() ? "Xem & phản hồi" : "Xem & sửa trả lời",
                  {988, 324.f + row * 230.f, 202, 36}, [this, id] { openEditor(id); });
    }
    addButton("Trước", {884, 630, 145, 36}, [this] {
        if (page_ > 0) --page_;
        rebuild_ = true;
    }).setEnabled(page_ > 0);
    addButton("Sau", {1055, 630, 145, 36}, [this] {
        ++page_;
        rebuild_ = true;
    }).setEnabled(page_ + 1 < pages);
}

void ReviewsState::openEditor(std::uint64_t id) {
    editing_ = id;
    auto *review = editingReview();
    if (!review) {
        editing_.reset();
        return;
    }
    draft_ = utf8(review->reply);
    inputFocused_ = true;
    selectAll_ = false;
    notice_.clear();
    noticeError_ = false;
    detailScroll_ = 0;
    blink_ = 0;
    rebuild_ = true;
}

void ReviewsState::closeEditor() {
    editing_.reset();
    inputFocused_ = false;
    selectAll_ = false;
    draft_.clear();
    rebuild_ = true;
}

void ReviewsState::submitReply() {
    auto *review = editingReview();
    if (!review) return;
    if (blank(draft_)) {
        notice_ = "Bạn hãy viết vài lời gửi khách trước nhé.";
        noticeError_ = true;
        return;
    }
    const std::string previous = review->reply;
    if (!ReviewSystem::reply(game().progress, review->id, asUtf8(draft_))) {
        notice_ = "Phản hồi cần có nội dung và tối đa 240 ký tự.";
        noticeError_ = true;
        return;
    }
    if (!game().save()) {
        review->reply = previous;
        notice_ = "Chưa lưu được. Nội dung của bạn vẫn ở đây để thử lại.";
        noticeError_ = true;
        return;
    }
    closeEditor();
    notice_ = "Đã lưu phản hồi của tiệm.";
    noticeError_ = false;
}

void ReviewsState::appendInput(const sf::String &input) {
    if (selectAll_) {
        draft_.clear();
        selectAll_ = false;
    }
    for (auto codepoint : input) {
        if (draft_.getSize() >= replyLimit) break;
        if (codepoint == '\n' || codepoint == '\r' || codepoint == '\t') codepoint = ' ';
        if (codepoint >= 32 && codepoint != 127 && codepoint != 0x2028 && codepoint != 0x2029)
            draft_ += codepoint;
    }
    blink_ = 0;
    notice_.clear();
}

bool ReviewsState::handleEvent(const sf::Event &event, Pointer mouse) {
    if (event.type == sf::Event::LostFocus) {
        onCovered();
        return false;
    }
    if (event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::Escape) {
        if (editing_) closeEditor();
        else stack_.pop();
        if (rebuild_) rebuildButtons();
        return false;
    }
    if (editing_) {
        if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left) {
            inputFocused_ = mouse && inputArea.contains(*mouse);
            selectAll_ = false;
        }
        if (event.type == sf::Event::MouseWheelScrolled && mouse && sf::FloatRect(220, 168, 840, 143).contains(*mouse)) {
            auto *review = editingReview();
            if (review) {
                const auto count = wrap(utf8(review->text), context_.resources.dialogFont(), 18, 812).size();
                const auto maxScroll = count > 6 ? count - 6 : 0;
                if (event.mouseWheelScroll.delta < 0) detailScroll_ = std::min(detailScroll_ + 1, maxScroll);
                else if (detailScroll_ > 0) --detailScroll_;
            }
        }
        if (inputFocused_ && event.type == sf::Event::KeyPressed) {
            if (event.key.code == sf::Keyboard::A && event.key.control)
                selectAll_ = true;
            else if (event.key.code == sf::Keyboard::C && event.key.control && selectAll_)
                sf::Clipboard::setString(draft_);
            else if (event.key.code == sf::Keyboard::V && event.key.control)
                appendInput(sf::Clipboard::getString());
            else if (event.key.code == sf::Keyboard::BackSpace && !draft_.isEmpty()) {
                if (selectAll_) draft_.clear();
                else draft_.erase(draft_.getSize() - 1, 1);
                selectAll_ = false;
                blink_ = 0;
                notice_.clear();
            } else if (event.key.code == sf::Keyboard::Enter) submitReply();
        }
        if (editing_ && inputFocused_ && event.type == sf::Event::TextEntered &&
            !sf::Keyboard::isKeyPressed(sf::Keyboard::LControl) && !sf::Keyboard::isKeyPressed(sf::Keyboard::RControl)) {
            const auto codepoint = event.text.unicode;
            if (codepoint >= 32 && codepoint != 127) appendInput(sf::String(codepoint));
        }
    } else if (event.type == sf::Event::MouseWheelScrolled && mouse && sf::FloatRect(320, 158, 890, 450).contains(*mouse)) {
        const auto size = filtered().size();
        if (event.mouseWheelScroll.delta < 0 && (page_ + 1) * perPage < size) ++page_;
        else if (event.mouseWheelScroll.delta > 0 && page_ > 0) --page_;
        rebuild_ = true;
    }
    // Callbacks only mark a rebuild; Button references stay valid for the entire dispatch.
    handleButtons(event, mouse);
    if (rebuild_) rebuildButtons();
    return false;
}

bool ReviewsState::update(float dt) {
    blink_ = std::fmod(blink_ + dt, 1.f);
    if (rebuild_) rebuildButtons();
    return UiState::update(dt);
}

void ReviewsState::onCovered() {
    UiState::onCovered();
    inputFocused_ = false;
}

void ReviewsState::drawEditor(sf::RenderTarget &target) {
    auto *review = editingReview();
    if (!review) return;
    const ui::Theme theme;
    const auto &font = context_.resources.uiFont();
    ui::panel(target, {180, 78, 920, 566}, theme.paper, 24);
    ui::text(target, context_.resources.titleFont(), "Lời nhắn gửi khách", 29, {220, 98}, theme.ink);
    ui::text(target, font, review->author + " • Ngày " + std::to_string(review->day), 18, {220, 141}, theme.ink);
    stars(target, review->stars, {915, 149}, 10, 28);
    ui::panel(target, {220, 174, 840, 140}, {247, 240, 222}, 12);
    paragraph(target, context_.resources.dialogFont(), utf8(review->text), 18, {232, 183}, 812, 21, 6, theme.ink, detailScroll_);
    if (wrap(utf8(review->text), context_.resources.dialogFont(), 18, 812).size() > 6)
        ui::text(target, font, "Cuộn chuột để đọc tiếp", 11, {900, 318}, theme.muted);
    ui::text(target, font, review->reply.empty() ? "Phản hồi từ tiệm" : "Chỉnh sửa phản hồi của tiệm", 19, {220, 337}, theme.green);
    ui::text(target, font, std::to_string(draft_.getSize()) + "/240", 14, {1000, 342}, theme.muted);
    ui::panel(target, {inputArea.left - 2, inputArea.top - 2, inputArea.width + 4, inputArea.height + 4},
              inputFocused_ ? theme.green : sf::Color(203, 185, 156), 12);
    ui::panel(target, inputArea, {255, 254, 247}, 11);
    const auto inputLines = wrap(draft_, font, 18, 810);
    const std::size_t firstInputLine = inputLines.size() > 5 ? inputLines.size() - 5 : 0;
    if (selectAll_ && inputFocused_) {
        for (std::size_t row = 0; row < inputLines.size() - firstInputLine; ++row) {
            sf::Text line(inputLines[firstInputLine + row], font, 18);
            ui::panel(target, {234, 380 + row * 23.f, line.getLocalBounds().width + 5, 23}, {207, 226, 190}, 0);
        }
    }
    if (draft_.isEmpty())
        ui::text(target, font, "Viết lời cảm ơn, xin lỗi hoặc ghi nhận góp ý của khách…", 18, {235, 383}, theme.muted);
    else paragraph(target, font, draft_, 18, {235, 383}, 810, 23, 5, theme.ink, firstInputLine);
    if (inputFocused_ && !selectAll_ && blink_ < .5f) {
        const std::size_t row = inputLines.size() - firstInputLine - 1;
        sf::Text last(inputLines.back(), font, 18);
        ui::panel(target, {235 + last.getLocalBounds().width + 2, 381 + row * 23.f, 1.5f, 21}, theme.green, 0);
    }
    ui::text(target, font, "Enter: lưu • Ctrl+V: dán nội dung • Esc: quay lại", 13, {220, 505}, theme.muted);
    if (!notice_.empty()) ui::text(target, font, notice_, 14, {220, 568}, noticeError_ ? sf::Color(168, 66, 45) : theme.green);
}

void ReviewsState::render(sf::RenderTarget &target) {
    const ui::Theme theme;
    const auto &font = context_.resources.uiFont();
    ui::panel(target, {0, 0, 1280, 720}, {36, 29, 21, 165}, 0);
    if (editing_) {
        drawEditor(target);
        renderButtons(target);
        return;
    }
    ui::panel(target, {40, 32, 1200, 656}, {255, 250, 235}, 26);
    ui::text(target, context_.resources.titleFont(), "Đánh giá của khách", 34, {70, 62}, theme.ink);
    ui::text(target, font, "Những lời nhắn sau một lần ghé tiệm", 17, {70, 111}, theme.muted);
    const auto &all = game().progress.reviews;
    std::array<int, 5> counts{};
    int total = 0;
    for (const auto &review : all) {
        ++counts[static_cast<std::size_t>(std::clamp(review.stars, 1, 5) - 1)];
        total += review.stars;
    }
    const float average = all.empty() ? 0.f : total / static_cast<float>(all.size());
    ui::panel(target, {70, 158, 225, 450}, {242, 233, 210}, 20);
    std::ostringstream score;
    score << std::fixed << std::setprecision(1) << average;
    ui::text(target, context_.resources.titleFont(), all.empty() ? "—" : score.str(), 53, {182, 216}, theme.ink, true);
    stars(target, all.empty() ? 0 : static_cast<int>(std::lround(average)), {126, 267}, 12, 28);
    ui::text(target, font, std::to_string(all.size()) + " đánh giá gần đây", 16, {182, 304}, theme.muted, true);
    const int largest = std::max(1, *std::max_element(counts.begin(), counts.end()));
    for (int row = 0; row < 5; ++row) {
        const int rating = 5 - row;
        const float y = 345 + row * 32.f;
        ui::text(target, font, std::to_string(rating), 16, {89, y}, theme.ink);
        ui::star(target, {114, y + 9}, 7, {227, 165, 54});
        ui::panel(target, {130, y + 5, 116, 9}, {221, 210, 185}, 4);
        const int count = counts[static_cast<std::size_t>(rating - 1)];
        if (count > 0) ui::panel(target, {130, y + 5, 116.f * count / largest, 9}, theme.green, 4);
        ui::text(target, font, std::to_string(count), 14, {257, y}, theme.muted);
    }
    paragraph(target, font, utf8("Một lời hồi đáp chân thành khiến khách muốn ghé lại."), 17, {88, 530}, 188, 23, 3, theme.green);
    const auto items = filtered();
    if (items.empty()) {
        ui::text(target, context_.resources.titleFont(), unansweredOnly_ ? "Bạn đã trả lời hết rồi!" : "Tiệm đang chờ những lời nhắn đầu tiên", 27,
                 {765, 311}, theme.green, true);
        paragraph(target, font, utf8(unansweredOnly_
                      ? "Khách sẽ rất vui khi biết tiệm đã lắng nghe. Chọn Tất cả để xem lại những cuộc trò chuyện."
                      : "Hãy mở quán và phục vụ vài phần bánh tráng. Một số khách sẽ ghé lại trang này để chia sẻ cảm nhận."),
                  19, {450, 357}, 650, 28, 3, theme.muted);
    }
    for (std::size_t row = 0; row < perPage; ++row) {
        const auto index = page_ * perPage + row;
        if (index >= items.size()) break;
        const auto &review = *items[index];
        const float y = 158 + row * 230.f;
        ui::panel(target, {320, y, 890, 220}, theme.paper, 18);
        ui::text(target, font, review.author, 20, {344, y + 17}, theme.ink);
        ui::text(target, font, "Ngày " + std::to_string(review.day) + " • Đã ghé tiệm", 13, {1000, y + 23}, theme.muted);
        stars(target, review.stars, {353, y + 54}, 8, 22);
        paragraph(target, context_.resources.dialogFont(), utf8(review.text), 16, {344, y + 73}, 842, 21, 3, theme.ink);
        if (!review.reply.empty()) {
            ui::panel(target, {344, y + 145, 620, 62}, {237, 242, 224}, 10);
            ui::text(target, font, "Tiệm trả lời", 12, {356, y + 151}, theme.green);
            paragraph(target, font, utf8(review.reply), 14, {356, y + 172}, 594, 17, 2, theme.ink);
        } else ui::text(target, font, "Khách đang chờ lời nhắn từ tiệm", 14, {344, y + 180}, theme.muted);
    }
    const auto pages = std::max<std::size_t>(1, (items.size() + perPage - 1) / perPage);
    ui::text(target, font, "Trang " + std::to_string(page_ + 1) + " / " + std::to_string(pages), 16, {760, 648}, theme.muted, true);
    if (!notice_.empty()) ui::text(target, font, notice_, 15, {70, 642}, noticeError_ ? sf::Color(168, 66, 45) : theme.green);
    renderButtons(target);
}
} // namespace tiem
