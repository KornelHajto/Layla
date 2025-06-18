# Layla: 2D Multiplayer Shooter (C/raylib)

A simple 2D multiplayer shooter game built with C and the raylib framework. Layla provides fast-paced action with a variety of weapons in a networked multiplayer environment.

## Features

- Multiple weapon types (Pistol, Rifle, Shotgun, SMG, Sniper)
- Network multiplayer with host/join capabilities
- Visual effects (particles, muzzle flashes, hit effects)
- Player movement with physics
- Basic damage system

## Project Structure

The project is organized into modules:

```
Layla/
├── include/           # Header files
│   ├── common.h       # Common definitions and structures
│   ├── core.h         # Core game functions
│   ├── network.h      # Networking functionality
│   ├── particles.h    # Particle system
│   ├── player.h       # Player management
│   └── weapons.h      # Weapons and bullets
├── src/               # Implementation files
│   ├── core.c         # Core game implementation
│   ├── main.c         # Entry point
│   ├── network.c      # Network implementation
│   ├── particles.c    # Particle system implementation
│   ├── player.c       # Player implementation
│   └── weapons.c      # Weapons implementation
├── Makefile           # Build configuration
└── README.md          # This file
```

## Building and Running

### Prerequisites

- GCC or Clang compiler
- raylib library (version 4.0 or higher)
- Standard development libraries

### Installation

#### Install raylib (Debian/Ubuntu):

```bash
sudo apt update
sudo apt install libraylib-dev libraylib4 raylib-examples
```

#### Install raylib from source:

```bash
make install-raylib
```

### Build the Game

```bash
# Regular build
make

# Debug build
make debug

# Optimized build
make release
```

### Running

```bash
# Run the game
./layla

# Or use the make target
make run
```

## Multiplayer Instructions

1. Host a game:
   - Press 'H' or '1' on the main menu
   - Enter the port number to host on
   - Click "START HOSTING"

2. Join a game:
   - Press 'J' or '2' on the main menu
   - Enter the host's IP address and port number
   - Click "CONNECT"

## Controls

- WASD or Arrow Keys: Move
- Mouse: Aim
- Mouse Left Button: Shoot
- R: Reload
- 1-5 or Mouse Wheel: Switch weapons
- ESC: Return to menu/Quit

## Debug Controls

- F1: Toggle debug mode
- F2: Toggle vsync
- F3: Cycle FPS limit
- F4: Toggle advanced stats
- F5: Toggle screen shake
- F6: Toggle smooth movement
- F7: Toggle visual effects

## License

This project is available under the MIT License.

## Credits

- Built with [raylib](https://www.raylib.com/)