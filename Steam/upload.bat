@echo off
setlocal

rem ===== Shards of Veyara - Steam upload (SteamPipe) =====
rem Keep this file ASCII-only with CRLF line endings (cmd.exe cannot parse UTF-8 / LF batch files reliably).

set "STEAMCMD=C:\steamcmd\steamcmd.exe"
set "SCRIPT_DIR=%~dp0scripts"
set "CONTENT_DIR=%~dp0content"

if not exist "%STEAMCMD%" (
  echo [ERROR] steamcmd.exe not found: %STEAMCMD%
  echo Edit the STEAMCMD path at the top of upload.bat.
  pause
  exit /b 1
)

if not exist "%CONTENT_DIR%\ShardsOfVeyara.exe" (
  echo [ERROR] ShardsOfVeyara.exe not found in Steam\content.
  echo Copy the contents of the packaged Windows folder into Steam\content.
  pause
  exit /b 1
)

findstr /c:"0000000" "%SCRIPT_DIR%\app_build.vdf" >nul
if not errorlevel 1 (
  echo [ERROR] AppID / Depot ID in app_build.vdf is still 0000000.
  pause
  exit /b 1
)

set /p "STEAM_USER=Steam account name: "
if "%STEAM_USER%"=="" (
  echo [ERROR] No account name entered.
  pause
  exit /b 1
)

echo.
echo Uploading with app_build.vdf ...
echo steamcmd will ask for the password and Steam Guard code.
echo.

"%STEAMCMD%" +login %STEAM_USER% +run_app_build "%SCRIPT_DIR%\app_build.vdf" +quit

echo.
echo Done. Go to Steamworks - SteamPipe - Builds and set the new build live on a branch.
pause