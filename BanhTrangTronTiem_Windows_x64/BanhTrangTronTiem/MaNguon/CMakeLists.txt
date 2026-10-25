cmake_minimum_required(VERSION 3.16)
project(BanhTrangTiem VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Giữ thời gian giải nén đúng trên CMake mới; vẫn tương thích CMake 3.16.
if(POLICY CMP0135)
    cmake_policy(SET CMP0135 NEW)
endif()
include(FetchContent)
set(BUILD_SHARED_LIBS OFF CACHE BOOL "Build static SFML" FORCE)
set(SFML_BUILD_AUDIO ON CACHE BOOL "" FORCE)
set(SFML_BUILD_GRAPHICS ON CACHE BOOL "" FORCE)
set(SFML_BUILD_WINDOW ON CACHE BOOL "" FORCE)
set(SFML_BUILD_NETWORK OFF CACHE BOOL "" FORCE)
set(SFML_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(SFML_BUILD_DOC OFF CACHE BOOL "" FORCE)
set(SFML_BUILD_TEST_SUITE OFF CACHE BOOL "" FORCE)
set(JSON_BuildTests OFF CACHE BOOL "" FORCE)
set(JSON_Install OFF CACHE BOOL "" FORCE)

# Khóa phiên bản và SHA-256: máy khác tải cùng một bộ mã nguồn.
FetchContent_Declare(sfml
    URL https://codeload.github.com/SFML/SFML/tar.gz/refs/tags/2.6.2
    URL_HASH SHA256=15ff4d608a018f287c6a885db0a2da86ea389e516d2323629e4d4407a7ce047f)
FetchContent_Declare(nlohmann_json
    URL https://codeload.github.com/nlohmann/json/tar.gz/refs/tags/v3.11.3
    URL_HASH SHA256=0d8ef5af7f9794e3263480193c491549b2ba6cc74bb018906202ada498a79406)
FetchContent_MakeAvailable(sfml nlohmann_json)

add_library(tiem_core STATIC
    src/gameplay/Model.cpp
    src/gameplay/Minigames.cpp
    src/core/SaveSystem.cpp
    src/core/AudioManager.cpp
    src/ui/Art.cpp
    src/ui/Effects.cpp
    src/core/AppConfig.cpp
    src/core/Paths.cpp
    src/core/ResourceManager.cpp
    src/core/StateStack.cpp
    src/ui/Button.cpp
    src/ui/Draw.cpp)
target_include_directories(tiem_core PUBLIC src)
target_link_libraries(tiem_core PUBLIC
    sfml-graphics sfml-window sfml-audio sfml-system nlohmann_json::nlohmann_json)

add_executable(BanhTrangTiem
    src/main.cpp
    src/core/Game.cpp
    src/states/MainMenuState.cpp
    src/states/PlayingState.cpp
    src/states/OverlayState.cpp
    src/states/ManagementState.cpp)
target_link_libraries(BanhTrangTiem PRIVATE tiem_core)
if(WIN32)
    # Ứng dụng đồ họa: mở game không kèm cửa sổ console.
    set_target_properties(BanhTrangTiem PROPERTIES WIN32_EXECUTABLE TRUE)
    target_link_libraries(BanhTrangTiem PRIVATE sfml-main)
endif()
# Mang runtime GCC theo executable, không cần cài compiler để chơi.
if(MINGW)
    target_link_options(BanhTrangTiem PRIVATE -static -static-libgcc -static-libstdc++)
endif()
set(TIEM_RUNTIME_DIR "${CMAKE_BINARY_DIR}/bin/$<CONFIG>")
set_target_properties(BanhTrangTiem PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${TIEM_RUNTIME_DIR}"
    VS_DEBUGGER_WORKING_DIRECTORY "$<TARGET_FILE_DIR:BanhTrangTiem>")

option(TIEM_SANITIZERS "Enable AddressSanitizer and UBSan for game code (GCC/Clang, Unix)" OFF)
function(tiem_warnings target)
    if(MSVC)
        target_compile_options(${target} PRIVATE /W4 /permissive- /utf-8)
        target_compile_definitions(${target} PRIVATE NOMINMAX WIN32_LEAN_AND_MEAN)
    else()
        target_compile_options(${target} PRIVATE -Wall -Wextra -Wpedantic)
        if(TIEM_SANITIZERS AND NOT MINGW)
            target_compile_options(${target} PRIVATE -fsanitize=address,undefined -fno-omit-frame-pointer)
            target_link_options(${target} PRIVATE -fsanitize=address,undefined)
        endif()
    endif()
endfunction()
tiem_warnings(tiem_core)
tiem_warnings(BanhTrangTiem)

# Luôn cập nhật dữ liệu khi build, kể cả khi chỉ sửa JSON/asset.
add_custom_target(tiem_runtime_files ALL
    COMMAND ${CMAKE_COMMAND} -E make_directory "${TIEM_RUNTIME_DIR}"
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_CURRENT_SOURCE_DIR}/assets" "${TIEM_RUNTIME_DIR}/assets"
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_CURRENT_SOURCE_DIR}/data" "${TIEM_RUNTIME_DIR}/data"
    VERBATIM)
add_dependencies(BanhTrangTiem tiem_runtime_files)

# SFML tĩnh vẫn dùng OpenAL động trên Windows.
if(WIN32)
    if(CMAKE_SIZEOF_VOID_P EQUAL 8)
        set(TIEM_OPENAL_ARCH x64)
    else()
        set(TIEM_OPENAL_ARCH x86)
    endif()
    add_custom_command(TARGET BanhTrangTiem POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${sfml_SOURCE_DIR}/extlibs/bin/${TIEM_OPENAL_ARCH}/openal32.dll"
            "$<TARGET_FILE_DIR:BanhTrangTiem>" VERBATIM)
endif()

option(TIEM_BUILD_TESTS "Build focused core tests" OFF)
if(TIEM_BUILD_TESTS)
    enable_testing()
    add_executable(tiem_tests tests/CoreTests.cpp)
    target_link_libraries(tiem_tests PRIVATE tiem_core)
    tiem_warnings(tiem_tests)
    if(MINGW)
        target_link_options(tiem_tests PRIVATE -static -static-libgcc -static-libstdc++)
    endif()
    if(WIN32)
        add_custom_command(TARGET tiem_tests POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${sfml_SOURCE_DIR}/extlibs/bin/${TIEM_OPENAL_ARCH}/openal32.dll"
                "$<TARGET_FILE_DIR:tiem_tests>" VERBATIM)
    endif()
    add_test(NAME core_behaviour COMMAND tiem_tests "${CMAKE_CURRENT_SOURCE_DIR}")
endif()
