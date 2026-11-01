#include "core/Paths.hpp"
#include <stdexcept>
#include <vector>
#ifdef _WIN32
#include <windows.h>
#elif defined(__APPLE__)
#include <cstdint>
#include <mach-o/dyld.h>
#endif

namespace tiem {
std::filesystem::path executableDirectory() {
#ifdef _WIN32
    // UTF-16 giúp chạy được khi thư mục Windows chứa dấu tiếng Việt.
    std::vector<wchar_t> buffer(32768);
    const DWORD size = GetModuleFileNameW(nullptr, buffer.data(), static_cast<DWORD>(buffer.size()));
    if (size == 0 || size >= buffer.size())
        throw std::runtime_error("Khong tim thay duong dan exe");
    return std::filesystem::path(std::wstring(buffer.data(), size)).parent_path();
#elif defined(__APPLE__)
    std::uint32_t size = 0;
    _NSGetExecutablePath(nullptr, &size);
    std::vector<char> buffer(size);
    if (_NSGetExecutablePath(buffer.data(), &size) != 0)
        throw std::runtime_error("Khong tim thay duong dan executable");
    return std::filesystem::weakly_canonical(buffer.data()).parent_path();
#else
    return std::filesystem::read_symlink("/proc/self/exe").parent_path();
#endif
}

std::filesystem::path findDataRoot() {
    const auto executable = executableDirectory();
    for (const auto &candidate : {executable, std::filesystem::current_path()}) {
        if (std::filesystem::is_regular_file(candidate / "data/app.json"))
            return candidate;
    }
    throw std::runtime_error("Thieu data/app.json. Hay giu assets/ va data/ canh file chay.");
}
} // namespace tiem
