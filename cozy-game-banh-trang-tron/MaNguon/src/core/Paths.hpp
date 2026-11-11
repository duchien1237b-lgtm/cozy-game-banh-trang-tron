#pragma once
#include <filesystem>

namespace tiem {
std::filesystem::path executableDirectory();
std::filesystem::path findDataRoot();
} // namespace tiem
