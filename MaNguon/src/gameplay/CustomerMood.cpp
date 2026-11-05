#include "gameplay/CustomerMood.hpp"
namespace tiem {
Expression CustomerMood::waiting(const Customer &c, const Personality &p) {
    // Vài giây đầu: khách vừa tới, tươi cười chào quán.
    if (c.waited() < 2.5f) return p.calm ? Expression::Neutral : Expression::Happy;
    const float mood = c.mood();
    if (mood < p.angerAt) return Expression::Angry;
    if (mood < p.worryAt) return p.calm ? Expression::Bored : Expression::Worried;
    if (p.cheerful) return Expression::Happy;
    return Expression::Neutral;
}
Expression CustomerMood::reaction(int stars, const Personality &p) {
    const int bar = p.strict > 1.05f ? 1 : 0; // khách kỹ tính cần thêm một sao mới vui
    if (stars >= 5) return Expression::Delighted;
    if (stars >= 4 + bar) return Expression::Happy;
    if (stars >= 3 + bar) return Expression::Neutral;
    return Expression::Sad;
}
const char *CustomerMood::face(Expression e) {
    switch (e) {
    case Expression::Delighted: case Expression::Happy: case Expression::Surprised: return "happy";
    case Expression::Angry: case Expression::Sad: return "angry";
    default: return "normal";
    }
}
const char *CustomerMood::label(Expression e) {
    switch (e) {
    case Expression::Delighted: return "Mê tít";
    case Expression::Happy: return "Vui vẻ";
    case Expression::Neutral: return "Bình thường";
    case Expression::Bored: return "Ngáp dài";
    case Expression::Worried: return "Sốt ruột";
    case Expression::Angry: return "Bực bội";
    case Expression::Sad: return "Thất vọng";
    case Expression::Surprised: return "Bất ngờ";
    }
    return "";
}
const std::string &CustomerMood::line(const Personality &p, Expression e, unsigned seed) {
    static const std::string fallback = "Cho mình một phần bánh tráng!";
    const std::vector<std::string> *pool = &p.greet;
    switch (e) {
    case Expression::Delighted: case Expression::Happy: pool = p.waiting.empty() ? &p.greet : &p.waiting; break;
    case Expression::Bored: case Expression::Worried: case Expression::Angry: pool = &p.hurry; break;
    default: pool = &p.greet; break;
    }
    if (pool->empty()) pool = &p.greet;
    if (pool->empty()) return fallback;
    return (*pool)[seed % pool->size()];
}
} // namespace tiem
