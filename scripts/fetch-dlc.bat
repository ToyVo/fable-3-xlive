rem SPDX-License-Identifier: Apache-2.0
@echo off
rem Fetch + install Fable 3 DLC packs for use with the unified xlive.dll.
rem Usage: scripts\fetch-dlc.bat [GAME_DIR]
rem   GAME_DIR defaults to the Steam install (or %%GAME_DIR%%).
rem   Env overrides: DLC_CACHE_DIR, BASE_PACK_DIR, FORCE=1 to re-extract.
setlocal EnableDelayedExpansion

if not "%~1"=="" set "GAME_DIR=%~1"
if "%GAME_DIR%"=="" set "GAME_DIR=%ProgramFiles(x86)%\Steam\steamapps\common\Fable 3"
if not exist "%GAME_DIR%\fable3.exe" if exist "C:\Steam\steamapps\common\Fable 3\fable3.exe" set "GAME_DIR=C:\Steam\steamapps\common\Fable 3"
if not exist "%GAME_DIR%\fable3.exe" (
  echo error: game not found under "%GAME_DIR%"
  echo usage: %~nx0 [GAME_DIR]
  exit /b 1
)

set "SCRIPT_DIR=%~dp0"
set "REPO_ROOT=%SCRIPT_DIR%.."
if "%BASE_PACK_DIR%"=="" set "BASE_PACK_DIR=%REPO_ROOT%\assets\90_DLC_xlive_removal"
if "%DLC_CACHE_DIR%"=="" set "DLC_CACHE_DIR=%REPO_ROOT%\.dlc-cache"
if "%FORCE%"=="" set "FORCE=0"
set "DLC_DIR=%GAME_DIR%\DLC"
set "CAB_BASE=http://download.xbox.com/content/4d53090a"

where curl >nul 2>nul
if errorlevel 1 (
  echo error: curl.exe not found (needs Windows 10+ or curl on PATH) >&2
  exit /b 1
)

if not exist "%DLC_DIR%" mkdir "%DLC_DIR%"
if not exist "%DLC_CACHE_DIR%" mkdir "%DLC_CACHE_DIR%"

rem 1. base stub pack (vendored safe spot; BASE_PACK_DIR to override)
if exist "%DLC_DIR%\90_DLC_xlive_removal" if not "%FORCE%"=="1" (
  echo keep: DLC\90_DLC_xlive_removal already present
) else if exist "%BASE_PACK_DIR%\content.xbx" (
  if exist "%DLC_DIR%\90_DLC_xlive_removal" rmdir /s /q "%DLC_DIR%\90_DLC_xlive_removal"
  xcopy "%BASE_PACK_DIR%" "%DLC_DIR%\90_DLC_xlive_removal\" /e /i /q /y >nul
  echo copy: 90_DLC_xlive_removal\ ^<- assets\
) else (
  echo warn: base pack missing at "%BASE_PACK_DIR%"; DLC needs
  echo       DLC\90_DLC_xlive_removal\ ^(see docs\DLC.md^)
)

rem 2-3. download + extract each pack: dir | cab | bytes | sha256-or-empty
call :pack 01_Understone      4d53090a0ecf0000 256943166  869a0147251b109a8c14ad842e875b381c93c8e6a883a064d9c29b904697caae || exit /b 1
call :pack 02_TraitorsKeep    4d53090a0ecf000e 1025720613 3728c92b4e4ef7f6c78fcf5720a98fcacb960d835d4e5c6d7ac5ff1d4fed9f79 || exit /b 1
call :pack 03_InquisitorsPack 4d53090a0ecf0010 1546685     836b94464b01f74e7cad6aeb7ec426b83aac12ecb180da6dfa9508730e136865 || exit /b 1

echo.
echo done. DLC layout under "%DLC_DIR%":
dir /b "%DLC_DIR%"
echo.
echo next: set 'profile xuid 1' in xlive.ini to your save-folder id, then run.
echo ^(unified xlive.dll + xlive.ini go next to fable3.exe - see docs\BUILD.md^)
exit /b 0

:pack
set "dir=%~1" & set "cab=%~2" & set "want=%~3" & set "sha=%~4"
set "dest=%DLC_DIR%\%dir%" & set "cabfile=%DLC_CACHE_DIR%\%cab%.cab"
if exist "%dest%\content.xbx" if not "%FORCE%"=="1" (
  echo keep: DLC\%dir% already extracted
  exit /b 0
)
echo == DLC\%dir%
set "got=0"
if exist "%cabfile%" for %%A in ("%cabfile%") do set "got=%%~zA"
if "!got!"=="%want%" (
  echo   have: cached cab, size OK
) else (
  echo   get:  %CAB_BASE%/%cab%.cab
  curl -fSL --retry 3 -C - -o "%cabfile%" "%CAB_BASE%/%cab%.cab" || exit /b 1
)
set "got=0"
for %%A in ("%cabfile%") do set "got=%%~zA"
if not "!got!"=="%want%" (
  echo error: size mismatch for %cab%.cab ^(got !got!, want %want%^) >&2
  exit /b 1
)
if not "%sha%"=="" (
  for /f "skip=1 tokens=*" %%H in ('certutil -hashfile "%cabfile%" SHA256 ^| findstr /v "CertUtil"') do set "gotsha=%%H"
  set "gotsha=!gotsha: =!"
  if /i not "!gotsha!"=="%sha%" (
    echo error: sha256 mismatch for %cab%.cab >&2
    exit /b 1
  )
  echo   ok:   sha256 verified
)
if not exist "%dest%" mkdir "%dest%"
echo   unpack -^> DLC\%dir%
expand "%cabfile%" -F:* "%dest%" >nul || exit /b 1
if not exist "%dest%\content.xbx" (
  echo error: content.xbx missing after unpack >&2
  exit /b 1
)
exit /b 0
