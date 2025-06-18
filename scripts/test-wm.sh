#!/bin/bash

# Layla Window Manager Testing Script
# This script helps test the window manager in various environments

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(dirname "$SCRIPT_DIR")"
WM_BINARY="$PROJECT_DIR/layla"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Logging functions
log_info() {
    echo -e "${BLUE}[INFO]${NC} $1"
}

log_success() {
    echo -e "${GREEN}[SUCCESS]${NC} $1"
}

log_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

log_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

# Check if running in X11
check_x11() {
    if [ -z "$DISPLAY" ]; then
        log_error "No X11 display found. Make sure you're running in an X11 session."
        return 1
    fi
    
    if ! command -v xwininfo >/dev/null 2>&1; then
        log_warning "xwininfo not found. Some tests may not work properly."
    fi
    
    return 0
}

# Build the window manager
build_wm() {
    log_info "Building window manager..."
    cd "$PROJECT_DIR"
    
    if [ "$1" = "debug" ]; then
        make debug
    else
        make
    fi
    
    if [ ! -f "$WM_BINARY" ]; then
        log_error "Build failed - binary not found"
        return 1
    fi
    
    log_success "Build completed successfully"
}

# Test in nested X session using Xephyr
test_nested() {
    local width=${1:-1024}
    local height=${2:-768}
    local display=":99"
    
    log_info "Starting nested X session (${width}x${height}) on display ${display}..."
    
    # Check if Xephyr is available
    if ! command -v Xephyr >/dev/null 2>&1; then
        log_error "Xephyr not found. Install it to test in nested session."
        log_info "On NixOS: nix-shell -p xorg.xserver"
        log_info "On Ubuntu: sudo apt install xserver-xephyr"
        return 1
    fi
    
    # Kill any existing Xephyr on this display
    pkill -f "Xephyr.*${display}" 2>/dev/null || true
    
    # Start Xephyr
    Xephyr -screen "${width}x${height}" "$display" -ac -reset -terminate &
    local xephyr_pid=$!
    
    # Wait for Xephyr to start
    sleep 2
    
    # Check if Xephyr started successfully
    if ! kill -0 $xephyr_pid 2>/dev/null; then
        log_error "Failed to start Xephyr"
        return 1
    fi
    
    log_success "Xephyr started on display ${display}"
    log_info "Starting window manager in nested session..."
    log_info "Press Ctrl+C to stop the test"
    
    # Start the window manager in the nested session
    DISPLAY="$display" "$WM_BINARY" &
    local wm_pid=$!
    
    # Start a terminal for testing
    sleep 1
    if command -v alacritty >/dev/null 2>&1; then
        DISPLAY="$display" alacritty &
    elif command -v xterm >/dev/null 2>&1; then
        DISPLAY="$display" xterm &
    fi
    
    # Wait for user to stop
    trap "kill $wm_pid $xephyr_pid 2>/dev/null || true" EXIT
    wait $wm_pid
}

# Test window manager directly (be careful!)
test_direct() {
    log_warning "This will replace your current window manager!"
    log_warning "Make sure you have a way to get back to your desktop."
    log_warning "Press Ctrl+C within 5 seconds to cancel..."
    
    sleep 5
    
    log_info "Starting window manager directly..."
    exec "$WM_BINARY"
}

# Check for another window manager
check_wm_running() {
    local wm_processes=("dwm" "i3" "awesome" "xmonad" "openbox" "fluxbox" "bspwm")
    
    for wm in "${wm_processes[@]}"; do
        if pgrep "$wm" >/dev/null 2>&1; then
            log_warning "Another window manager ($wm) is running"
            log_info "You may need to stop it first: pkill $wm"
            return 1
        fi
    done
    
    return 0
}

# Run basic functionality tests
test_functionality() {
    log_info "Running basic functionality tests..."
    
    # Test if binary exists and is executable
    if [ ! -x "$WM_BINARY" ]; then
        log_error "Window manager binary is not executable"
        return 1
    fi
    
    # Test help output (if implemented)
    if "$WM_BINARY" --help >/dev/null 2>&1; then
        log_success "Help option works"
    fi
    
    # Test version output (if implemented)
    if "$WM_BINARY" --version >/dev/null 2>&1; then
        log_success "Version option works"
    fi
    
    log_success "Basic functionality tests passed"
}

# Show help
show_help() {
    cat << EOF
Layla Window Manager Test Script

Usage: $0 [COMMAND] [OPTIONS]

Commands:
    build [debug]    Build the window manager (debug for debug build)
    test-nested      Test in nested X session (safe)
    test-direct      Test by replacing current WM (DANGEROUS!)
    check            Run basic checks and tests
    help             Show this help message

Options for test-nested:
    -s WIDTHxHEIGHT  Set screen size (default: 1024x768)

Examples:
    $0 build debug          # Build with debug symbols
    $0 test-nested          # Test in 1024x768 nested window
    $0 test-nested -s 800x600  # Test in smaller window
    $0 check                # Run basic checks

Safety Notes:
    - Use 'test-nested' for safe testing
    - 'test-direct' will replace your current window manager
    - Always have a backup way to access your desktop
    - Test in a virtual machine if unsure

Key Bindings to Test:
    Super + Return          Spawn terminal
    Super + q               Close window
    Super + Shift + q       Quit window manager
    Super + Mouse drag      Move windows
EOF
}

# Main script logic
main() {
    case "${1:-help}" in
        "build")
            cd "$PROJECT_DIR"
            build_wm "$2"
            ;;
        "test-nested")
            if ! check_x11; then
                exit 1
            fi
            
            if [ ! -f "$WM_BINARY" ]; then
                log_info "Binary not found, building first..."
                build_wm
            fi
            
            # Parse screen size option
            if [ "$2" = "-s" ] && [ -n "$3" ]; then
                IFS='x' read -r width height <<< "$3"
                test_nested "$width" "$height"
            else
                test_nested
            fi
            ;;
        "test-direct")
            if ! check_x11; then
                exit 1
            fi
            
            if [ ! -f "$WM_BINARY" ]; then
                log_info "Binary not found, building first..."
                build_wm
            fi
            
            check_wm_running
            test_direct
            ;;
        "check")
            log_info "Running comprehensive checks..."
            
            # Check build environment
            if ! command -v gcc >/dev/null 2>&1; then
                log_error "GCC not found"
                exit 1
            fi
            
            if ! pkg-config --exists x11 2>/dev/null; then
                log_error "X11 development libraries not found"
                exit 1
            fi
            
            # Build and test
            build_wm
            test_functionality
            
            log_success "All checks passed!"
            log_info "Ready to test with: $0 test-nested"
            ;;
        "help"|"-h"|"--help")
            show_help
            ;;
        *)
            log_error "Unknown command: $1"
            show_help
            exit 1
            ;;
    esac
}

# Run main function with all arguments
main "$@"