@echo off
setlocal
if "%QT_DIR%"=="" (
  echo Set QT_DIR to your Qt kit directory first.
  exit /b 1
)
set CLIENT=build\Release\tcpchat_client.exe
set SERVER=build\Release\tcpchat_server.exe
if not exist "%CLIENT%" set CLIENT=build\tcpchat_client.exe
if not exist "%SERVER%" set SERVER=build\tcpchat_server.exe
if not exist "%CLIENT%" (
  echo Could not find tcpchat_client.exe. Build the project first.
  exit /b 1
)
if not exist "%SERVER%" (
  echo Could not find tcpchat_server.exe. Build the project first.
  exit /b 1
)
mkdir dist\client 2>nul
mkdir dist\server 2>nul
copy /Y "%CLIENT%" dist\client\ >nul
copy /Y "%SERVER%" dist\server\ >nul
"%QT_DIR%\bin\windeployqt.exe" dist\client\tcpchat_client.exe
if errorlevel 1 exit /b %errorlevel%
"%QT_DIR%\bin\windeployqt.exe" --no-plugins dist\server\tcpchat_server.exe
if errorlevel 1 exit /b %errorlevel%
echo.
echo Portable app folders created: dist\client and dist\server
