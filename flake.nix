{
  description = "Mozart";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";
  };

  outputs = { self, nixpkgs }:
    let
      supportedSystems = [ "x86_64-linux" "aarch64-linux" "x86_64-darwin" "aarch64-darwin" ];
      forAllSystems = nixpkgs.lib.genAttrs supportedSystems;
    in
    {
      packages = forAllSystems (system:
        let
          pkgs = nixpkgs.legacyPackages.${system};
        in {
          mozart = pkgs.stdenv.mkDerivation {
            pname = "mozart";
            version = "unstable";
            src = ./.;

            buildInputs = with pkgs; [
              ftxui
              libvlc
            ];

            buildPhase = ''
              runHook preBuild
              make
              runHook postBuild
            '';

            installPhase = ''
              runHook preInstall
              install -Dm755 mozart $out/bin/mozart
              runHook postInstall
            '';

            meta = with pkgs.lib; {
              description = "Minimal and suckless TUI music player";
              homepage = "https://github.com/thestaccato/mozart";
              license = licenses.gpl3;
              platforms = platforms.unix;
              mainProgram = "mozart";
            };
          };

          default = self.packages.${system}.mozart;
        }
      );

      devShells = forAllSystems (system:
        let
          pkgs = nixpkgs.legacyPackages.${system};
        in {
          default = pkgs.mkShell {
            inputsFrom = [ self.packages.${system}.mozart ];
            packages = with pkgs; [
              gdb
            ];
          };
        }
      );
    };
}
