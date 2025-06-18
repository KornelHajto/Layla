{
  description = "Layla X11 Window Manager";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";
    flake-utils.url = "github:numtide/flake-utils";
  };

  outputs = { self, nixpkgs, flake-utils }:
    flake-utils.lib.eachDefaultSystem (system:
      let
        pkgs = nixpkgs.legacyPackages.${system};
      in
      {
        packages.default = pkgs.stdenv.mkDerivation {
          pname = "layla";
          version = "0.1.0";
          
          src = ./.;
          
          buildInputs = with pkgs; [
            xorg.libX11
          ];
          
          nativeBuildInputs = with pkgs; [
            gcc
            pkg-config
          ];
          
          buildPhase = ''
            make release
          '';
          
          installPhase = ''
            mkdir -p $out/bin
            cp layla $out/bin/
          '';
          
          meta = with pkgs.lib; {
            description = "A simple X11 window manager";
            homepage = "https://github.com/YOUR_USERNAME/layla";
            license = licenses.mit;
            maintainers = [ ];
            platforms = platforms.linux;
          };
        };

        devShells.default = pkgs.mkShell {
          buildInputs = with pkgs; [
            # Build dependencies
            gcc
            pkg-config
            xorg.libX11
            
            # Development tools
            gdb
            valgrind
            clang-tools
            cppcheck
            
            # X11 testing tools
            xorg.xwininfo
            xorg.xprop
            xorg.xev
            
            # Terminal emulators for testing
            alacritty
            xterm
          ];
          
          shellHook = ''
            echo "Layla Window Manager Development Environment"
            echo "==========================================="
            echo "Available commands:"
            echo "  make          - Build the window manager"
            echo "  make debug    - Build with debug symbols"
            echo "  make release  - Build optimized version"
            echo "  make clean    - Clean build artifacts"
            echo "  make format   - Format source code"
            echo "  make lint     - Run static analysis"
            echo ""
            echo "To test the window manager:"
            echo "  1. Build: make"
            echo "  2. Run in nested X session or switch to TTY"
            echo "  3. Start: startx ./layla"
            echo ""
            echo "Key bindings:"
            echo "  Super + Return    - Spawn terminal"
            echo "  Super + q         - Close focused window"
            echo "  Super + Shift + q - Quit window manager"
            echo "  Super + mouse     - Move/resize windows"
          '';
        };

        # NixOS module for system integration
        nixosModules.default = { config, lib, pkgs, ... }:
          with lib;
          let
            cfg = config.services.layla;
          in {
            options.services.layla = {
              enable = mkEnableOption "Layla window manager";
              
              package = mkOption {
                type = types.package;
                default = self.packages.${system}.default;
                description = "Layla package to use";
              };
            };

            config = mkIf cfg.enable {
              services.xserver = {
                enable = true;
                windowManager.layla = {
                  enable = true;
                  package = cfg.package;
                };
              };
              
              environment.systemPackages = [ cfg.package ];
            };
          };
      });
}