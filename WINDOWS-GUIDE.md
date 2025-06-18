# 🎮 Layla - Windows Installation Guide

## 🚀 Quick Start (Easiest Method)

### Option 1: Use Pre-built Executable (If Available)
If someone has already built `layla.exe` for you:
1. Download `layla.exe`
2. Double-click to run
3. Enjoy the game! 🎉

### Option 2: Build from Source

#### Step 1: Install MSYS2 (Recommended)
1. Download MSYS2 from: https://www.msys2.org/
2. Install it (default settings are fine)
3. Open "MSYS2 MSYS" from Start Menu

#### Step 2: Install Dependencies
```bash
# Update package database
pacman -Syu

# Install build tools and raylib
pacman -S mingw-w64-x86_64-gcc mingw-w64-x86_64-raylib make unzip
```

#### Step 3: Build the Game
1. Extract the game source code
2. Open MSYS2 terminal
3. Navigate to the game directory
4. Run: `make -f Makefile.windows`
5. Wait for compilation to finish
6. Run: `./layla.exe`

## 🎯 Alternative: Visual Studio

If you prefer Visual Studio:
1. Install Visual Studio 2022 (Community is free)
2. Install "Desktop development with C++" workload
3. Download raylib from: https://github.com/raysan5/raylib/releases
4. Create new "Empty Project"
5. Add all `.c` files from `src/` folder
6. Configure include directories and link raylib
7. Build and run

## 🎮 Game Controls

- **WASD**: Move your character
- **Mouse**: Aim and shoot
- **R**: Reload weapon
- **TAB**: View scoreboard (hold)
- **ENTER**: Open chat
- **ESC**: Return to menu
- **F1**: Toggle debug info
- **F2**: Toggle VSync

## 🌐 Multiplayer Setup

### Host a Game:
1. Choose "Host Game" from menu
2. Enter your player name
3. Set port (default 7777 is fine)
4. Click "START HOSTING"
5. Share your IP address with friends

### Join a Game:
1. Choose "Join Game" from menu
2. Enter your player name
3. Enter host's IP address
4. Enter port (usually 7777)
5. Click "CONNECT"

## 🛠️ Troubleshooting

### Build Issues:
- Make sure MSYS2 is updated: `pacman -Syu`
- Verify raylib is installed: `pacman -S mingw-w64-x86_64-raylib`
- Check that gcc is available: `gcc --version`

### Runtime Issues:
- **Black screen**: Update graphics drivers
- **Can't connect**: Check firewall settings
- **Lag**: Try hosting on a different port
- **Crashes**: Run in compatibility mode

### Firewall Setup:
Windows may block network connections:
1. Go to Windows Defender Firewall
2. Click "Allow an app through firewall"
3. Add `layla.exe` to the list
4. Enable for both Private and Public networks

## 📁 File Structure
```
layla-game/
├── src/           # Source code files
├── include/       # Header files
├── Makefile.windows # Windows build configuration
├── build.bat      # Easy build script
└── README-WINDOWS.md # This file
```

## 🆘 Need Help?

If you run into issues:
1. Check that all dependencies are installed
2. Try running `build.bat` instead of manual compilation
3. Make sure Windows Defender isn't blocking the executable
4. Update your graphics drivers

## 🎊 Have Fun!

Once you get it running, enjoy the fast-paced multiplayer action! The game features:
- Multiple weapon types
- Team-based game modes
- Real-time chat system
- Detailed statistics
- Smooth 60+ FPS gameplay

Happy gaming! 🎮✨
