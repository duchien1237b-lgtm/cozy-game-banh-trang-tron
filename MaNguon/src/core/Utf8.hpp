#pragma once
#include <SFML/System/String.hpp>
#include <string_view>

namespace tiem {
inline sf::String utf8(std::string_view value) {
    return sf::String::fromUtf8(value.begin(), value.end());
}
} // namespace tiem
