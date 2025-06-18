# Layla Window Manager

A lightweight, minimalist X11 window manager written in C, designed for simplicity and efficiency on NixOS and other Linux distributions.

## Features

- **Lightweight**: Minimal resource usage with a small codebase
- **Floating window management**: Simple click-to-focus and drag-to-move interface
- **Customizable**: Easy configuration through header files
- **Keyboard shortcuts**: Efficient window management with configurable key bindings
- **NixOS integration**: First-class support with Nix flakes and NixOS modules
- **Focus follows mouse**: Optional mouse-driven focus management
- **Window borders**: Visual feedback for focused and unfocused windows

## Key Bindings (Default)

| Key Combination | Action |
|----------------|--------|
| `Super + Return` | Spawn terminal |
| `Super + d` | Run application launcher (dmenu) |
| `Super + w` | Open web browser |
| `Super + f` | Open file manager |
| `Super + q` | Close focused window |
| `Super + Shift + q` | Quit window manager |
| `Super + F11` | Toggle fullscreen |
| `Super + Tab` | Next window |
| `Super + Shift + Tab` | Previous window |
| `Super + Mouse1` | Move window |
| `Super + Mouse3` | Resize window |

## Installation

### Using Nix Flakes (Recommended for NixOS)

1. Clone the repository:
```bash
git clone https://github.com/YOUR_USERNAME/layla.git
cd layla
```

2. Enter the development shell:
```bash
nix develop
```

3. Build the window manager:
```bash
make release
```

4. Install system-wide (optional):
```bash
nix build
sudo cp result/bin/layla /usr/local/bin/
```

### Traditional Build

1. Install dependencies:
   - X11 development libraries (`libX11-dev` on Debian/Ubuntu, `xorg-dev` on Arch)
   - GCC or Clang compiler
   - Make

2. Clone and build:
```bash
git clone https://github.com/YOUR_USERNAME/layla.git
cd layla
make
```

3. Install:
```bash
sudo make install
```

### NixOS System Integration

Add to your NixOS configuration:

```nix
{
  inputs.layla.url = "github:YOUR_USERNAME/layla";
  
  outputs = { self, nixpkgs, layla }: {
    nixosConfigurations.yourhostname = nixpkgs.lib.nixosSystem {
      modules = [
        layla.nixosModules.default
        {
          services.layla.enable = true;
          services.xserver.enable = true;
          services.xserver.displayManager.gdm.enable = true;
        }
      ];
    };
  };
}
```

## Usage

### Starting Layla

#### From Display Manager
Select "Layla" from your display manager's session list.

#### From TTY/Console
```bash
startx layla
```

#### Testing in Nested X Session
For testing without logging out:
```bash
Xephyr -screen 1024x768 :1 &
DISPLAY=:1 layla
```

### First Steps

1. **Open a terminal**: Press `Super + Return`
2. **Launch applications**: Press `Super + d` for dmenu
3. **Move windows**: Hold `Super` and drag with left mouse button
4. **Resize windows**: Hold `Super` and drag with right mouse button
5. **Close windows**: Press `Super + q`
6. **Quit**: Press `Super + Shift + q`

## Configuration

### Basic Configuration

Edit `src/config.h` to customize:

- **Key bindings**: Modify the `keys[]` array
- **Colors**: Change `BORDER_COLOR` and `BORDER_FOCUS_COLOR`
- **Border width**: Adjust `BORDER_WIDTH`
- **Default applications**: Set `TERMINAL_CMD`, `BROWSER_CMD`, etc.

Example customization:

```c
#define MOD_KEY Mod1Mask        // Use Alt instead of Super
#define BORDER_WIDTH 3          // Thicker borders
#define BORDER_COLOR 0x2E3440   // Nord theme colors
#define BORDER_FOCUS_COLOR 0x81A1C1
```

### Advanced Configuration

- **Window rules**: Define application-specific behavior in the `rules[]` array
- **Mouse bindings**: Customize `MOVE_BUTTON` and `RESIZE_BUTTON`
- **Focus behavior**: Toggle `FOCUS_FOLLOWS_MOUSE` and `CLICK_TO_FOCUS`

After making changes, rebuild:
```bash
make clean && make
```

## Development

### Development Environment

Using Nix (recommended):
```bash
nix develop
```

Manual setup:
```bash
# Install development dependencies
sudo apt install build-essential libx11-dev libxutil-dev gdb valgrind

# Or on Arch:
sudo pacman -S base-devel libx11 gdb valgrind
```

### Building and Testing

```bash
# Debug build
make debug

# Release build
make release

# Run static analysis
make lint

# Format code
make format

# Clean build artifacts
make clean
```

### Debugging

1. **Build with debug symbols**:
```bash
make debug
```

2. **Run with GDB**:
```bash
gdb ./layla
```

3. **Memory checking with Valgrind**:
```bash
valgrind --tool=memcheck ./layla
```

### Architecture

Layla follows a simple event-driven architecture:

- `wm_init()`: Initialize X11 connection and grab keys/buttons
- `wm_run()`: Main event loop processing X11 events
- Event handlers: Process specific X11 events (MapRequest, KeyPress, etc.)
- Window management: Focus, move, resize, and close windows

## Troubleshooting

### Common Issues

**"Cannot open display"**
- Ensure X11 is running: `echo $DISPLAY`
- Check X11 permissions: `xhost +local:`

**"Another window manager is running"**
- Kill existing window manager: `pkill dwm` or `pkill i3`
- Or test in nested session: `Xephyr -screen 800x600 :1 & DISPLAY=:1 layla`

**Applications don't start**
- Check if applications are installed: `which alacritty`
- Modify terminal command in `config.h`

**Key bindings don't work**
- Verify key names in `config.h`
- Check if keys are grabbed by other applications
- Test with `xev` to see key events

### Debugging X11 Issues

```bash
# Check X11 events
xev

# View window properties
xwininfo
xprop

# Check for X11 errors
layla 2>&1 | grep "X Error"
```

## Contributing

1. Fork the repository
2. Create a feature branch: `git checkout -b feature-name`
3. Make changes and test thoroughly
4. Follow the existing code style (use `make format`)
5. Submit a pull request

### Code Style

- Use 4-space indentation
- Follow K&R brace style
- Keep functions under 50 lines when possible
- Add comments for complex logic
- Use meaningful variable names

## License

MIT License - see LICENSE file for details.

## Acknowledgments

- Inspired by dwm, i3, and other minimalist window managers
- Built with X11/Xlib documentation and tutorials
- Thanks to the NixOS community for packaging guidance

## Roadmap

- [ ] Tiling layout support
- [ ] Multi-monitor support
- [ ] Status bar integration
- [ ] Workspace/tag system
- [ ] Configuration file support (runtime)
- [ ] Window animations
- [ ] System tray support
- [ ] EWMH compliance improvements

---

For more information, visit the [project homepage](https://github.com/YOUR_USERNAME/layla) or check the [wiki](https://github.com/YOUR_USERNAME/layla/wiki).