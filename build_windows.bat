@echo off
setlocal
if "%QT_DIR%"=="" (
  echo Set QT_DIR to your Qt kit directory first.
  echo Example for PowerShell: $env:QT_DIR="C:\Qt\6.8.3\msvc2022_64"
  exit /b 1
)
cmake -S . -B build -G "Visual Studio 17 2022" -DCMAKE_PREFIX_PATH="%QT_DIR%" -A x64
if errorlevel 1 exit /b %errorlevel%
cmake --build build --config Release --parallel
if errorlevel 1 exit /b %errorlevel%
echo.
echo Build complete. Look in build\Release\ for tcpchat_client.exe and tcpchat_server.exe.
