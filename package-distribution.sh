#!/bin/bash
# Package distribution script

echo "📦 Creating Layla distribution package..."

# Create distribution directory
DIST_DIR="layla-distribution"
rm -rf $DIST_DIR
mkdir -p $DIST_DIR

# Copy source files
echo "📁 Copying source files..."
cp -r src/ include/ Makefile README.md $DIST_DIR/

# Create Windows-specific files
cp Makefile.windows build-windows.sh $DIST_DIR/

# Create a simple README for Windows users
cat > $DIST_DIR/README-WINDOWS.md << 'EOF'
# Layla - Windows Build Instructions

## Quick Start (Recommended)
1. Install MSYS2 from: https://www.msys2.org/
2. Open MSYS2 terminal and run:
   ```bash
   pacman -S mingw-w64-x86_64-gcc mingw-w64-x86_64-raylib make
   ```
3. Navigate to this directory and run:
   ```bash
   make -f Makefile.windows
   ```

## Alternative: Visual Studio
1. Install Visual Studio with C++ support
2. Download raylib from: https://github.com/raysan5/raylib/releases
3. Create a new C++ project and add all .c files from src/
4. Configure include paths to point to include/ and raylib headers
5. Link against raylib library

## Pre-built Binary
If someone has already built `layla.exe`, just double-click to run!

## Controls
- WASD: Move
- Mouse: Aim and shoot
- R: Reload
- TAB: Scoreboard
- ENTER: Chat
- ESC: Menu
EOF

# Create a batch file for easy Windows building
cat > $DIST_DIR/build.bat << 'EOF'
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
EOF

# Create archive
echo "🗜️ Creating distribution archive..."
tar -czf layla-game.tar.gz $DIST_DIR

echo "✅ Distribution package created:"
echo "   📁 Directory: $DIST_DIR/"
echo "   📦 Archive: layla-game.tar.gz"
echo ""
echo "🚀 To share with your Windows friend:"
echo "   1. Send them the layla-game.tar.gz file"
echo "   2. They extract it and follow README-WINDOWS.md"
echo "   3. Or use the build.bat file for easy building"
