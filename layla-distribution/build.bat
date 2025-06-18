@echo off
echo Building Layla for Windows...

REM Check if we're in MSYS2 environment
where gcc >nul 2>nul
if %ERRORLEVEL% NEQ 0 (
    echo Error: GCC not found. Please install MSYS2 and run this from MSYS2 terminal.
    echo Download from: https://www.msys2.org/
    pause
    exit /b 1
)

REM Build the game
make -f Makefile.windows

if %ERRORLEVEL% EQU 0 (
    echo.
    echo Build successful! Created layla.exe
    echo Double-click layla.exe to play!
) else (
    echo Build failed. Check error messages above.
)

pause
