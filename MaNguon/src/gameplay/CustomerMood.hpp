#pragma once
#include "gameplay/Model.hpp"
namespace tiem {
// Biểu cảm hiển thị của khách. Ảnh gốc có ba nét mặt (happy/normal/angry); mỗi biểu cảm chọn
// một nét mặt nền rồi phần vẽ thêm biểu tượng cảm xúc và kiểu chuyển động riêng.
enum class Expression { Delighted, Happy, Neutral, Bored, Worried, Angry, Sad, Surprised };
class CustomerMood {
  public:
    // Biểu cảm khi đang chờ, theo kiên nhẫn còn lại và tính cách.
    static Expression waiting(const Customer &, const Personality &);
    // Phản ứng khi nhận món theo số sao (khách khó tính khắt khe hơn).
    static Expression reaction(int stars, const Personality &);
    static const char *face(Expression);  // tên ảnh nền: happy / normal / angry
    static const char *label(Expression); // nhãn tiếng Việt
    // Câu thoại theo tính cách và biểu cảm; seed giữ câu cố định cho mỗi khách.
    static const std::string &line(const Personality &, Expression, unsigned seed);
};
} // namespace tiem
