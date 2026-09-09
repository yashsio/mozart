# Mozart - terminal music player

A fast, minimal and suckless TUI music player.
On Linux, Mozart registers itself as `org.mpris.MediaPlayer2.mozart` on the
session bus, so it shows up in MPRIS-aware status bars (e.g. Waybar's `mpris`
module) and media control tools. On other platforms it builds without MPRIS.

    make MPRIS=1    # Linux build with MPRIS (default on Linux)
    make MPRIS=0    # universal build without MPRIS

## Dependencies

- libVLC
- FTXUI
- C++20 compiler
- sdbus-c++ 2.x (Linux only, for MPRIS support)

## Installation

### NixOS

#### User level installation

Run the following command from the directory containing flake.nix

    nix profile install .#mozart

#### System level installation

Just add the following block in your configuration.nix :

    { config, pkgs, ... }:
    let
      mozartFlake = builtins.getFlake "/full/path/to/your/flake";
    in {
      environment.systemPackages = [
        mozartFlake.packages.${pkgs.system}.mozart
      ];
    }

And if you prefer flakes, add this flake as a dependency and include mozart in system packages:

    {
      inputs = {
        mozart.url = "github:yashsio/mozart";
      };

      outputs = { self, nixpkgs, mozart, ... }:
        {
          nixosConfigurations.myMachine = nixpkgs.lib.nixosSystem {
            modules = [
              ({ pkgs, ... }: {
                environment.systemPackages = [
                  mozart.packages.${pkgs.system}.default
                ];
              })
            ];
          };
        };
    }

Or build from a local checkout (not recommended):

    nix build
    sudo cp ./result/bin/mozart /usr/local/bin/

#### Local run

To build and run without installing:

    nix build
    ./result/bin/mozart

Or with make (have to use "nix develop" command first):

    make && ./mozart

### Other Distributions

Just run the following command (if necessary as root) : 

    make clean install

## License
Licensed under [GPL-3.0](LICENSE)
