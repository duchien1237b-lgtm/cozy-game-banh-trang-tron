@echo off
setlocal
cd /d "%~dp0"
where cmake >nul 2>nul
if errorlevel 1 goto missing
where g++ >nul 2>nul
if errorlevel 1 goto missing
where mingw32-make >nul 2>nul
if errorlevel 1 goto missing
cmake -S . -B build-mingw -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release
if errorlevel 1 goto failed
cmake --build build-mingw --parallel 4
if errorlevel 1 goto failed
echo Build xong: build-mingw\bin\Release\BanhTrangTiem.exe
start "" "%~dp0build-mingw\bin\Release\BanhTrangTiem.exe"
exit /b 0
:missing
echo Can CMake, g++ va mingw32-make trong PATH. Ban exe co san khong can cac cong cu nay.
goto failed
:failed
echo Build chua thanh cong. Giu cua so nay de xem thong bao loi phia tren.
pause
exit /b 1
