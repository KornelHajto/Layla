#!/bin/bash
# Build script for creating Windows executable

echo "🪟 Building Layla for Windows..."

# Check if MinGW is installed
if ! command -v x86_64-w64-mingw32-gcc &> /dev/null; then
    echo "❌ MinGW-w64 not found. Installing..."
    
    # Detect package manager and install
    if command -v apt &> /dev/null; then
        sudo apt update && sudo apt install -y mingw-w64 mingw-w64-tools
    elif command -v dnf &> /dev/null; then
        sudo dnf install -y mingw64-gcc mingw64-gcc-c++
    elif command -v pacman &> /dev/null; then
        sudo pacman -S mingw-w64-gcc
    else
        echo "❌ Could not detect package manager. Please install MinGW-w64 manually."
        exit 1
    fi
fi

# Create raylib directory structure
mkdir -p raylib-windows/{lib,include}

# Check if raylib Windows libraries exist
if [ ! -f "raylib-windows/lib/libraylib.a" ]; then
    echo "📦 Downloading raylib for Windows..."
    
    # Download raylib Windows release
    RAYLIB_VERSION="5.0"
    RAYLIB_URL="https://github.com/raysan5/raylib/releases/download/${RAYLIB_VERSION}/raylib-${RAYLIB_VERSION}_win64_mingw-w64.zip"
    
    # Download and extract
    wget -O raylib-windows.zip "$RAYLIB_URL" 2>/dev/null || {
        echo "❌ Could not download raylib. Please download manually from:"
        echo "   https://github.com/raysan5/raylib/releases"
        echo "   Extract to raylib-windows/ directory"
        exit 1
    }
    
    unzip -q raylib-windows.zip -d raylib-temp
    cp -r raylib-temp/raylib-${RAYLIB_VERSION}_win64_mingw-w64/lib/* raylib-windows/lib/
    cp -r raylib-temp/raylib-${RAYLIB_VERSION}_win64_mingw-w64/include/* raylib-windows/include/
    rm -rf raylib-temp raylib-windows.zip
    
    echo "✅ raylib for Windows installed"
fi

# Compile for Windows
echo "🔨 Compiling for Windows..."

x86_64-w64-mingw32-gcc -std=c99 -Wall -Wextra -O3 \
    -I./include -I./raylib-windows/include \
    src/*.c \
    -o layla.exe \
    -L./raylib-windows/lib -lraylib \
    -lopengl32 -lgdi32 -lwinmm -lws2_32 \
    -static-libgcc -static-libstdc++ -static

if [ $? -eq 0 ]; then
    echo "✅ Windows build successful!"
    echo "📁 Created: layla.exe"
    echo "📦 File size: $(ls -lh layla.exe | awk '{print $5}')"
    echo ""
    echo "🚀 To run on Windows:"
    echo "   1. Copy layla.exe to your Windows friend's computer"
    echo "   2. Double-click to run (no installation needed!)"
else
    echo "❌ Build failed"
    exit 1
fi
