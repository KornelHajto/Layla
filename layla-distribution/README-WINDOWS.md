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
