{
  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs";
  };
  outputs =
    {
      self,
      nixpkgs,
    }:
    {
      packages.x86_64-linux = with (import nixpkgs { system = "x86_64-linux"; }); rec {
        csi = rustPlatform.buildRustPackage {
          name = "csi";
          src = ./csi;
          cargoLock = {
            lockFile = ./csi/Cargo.lock;
          };
          nativeBuildInputs = [ protobuf ];
        };
        csi-node = dockerTools.streamLayeredImage {
          name = "csi-node";
          tag = "latest";
          contents = [
            csi
            interposer
          ];
          config.Entrypoint = [ "/bin/csi-node" ];
        };
        interposer = stdenv.mkDerivation {
          name = "interposer";
          src = ./fuse;
          nativeBuildInputs = [
            meson
            ninja
            cmake
            pkg-config
          ];
          buildInputs = [
            fuse3
            (opentelemetry-cpp.overrideAttrs rec {
              version = "1.17.0";
              src = fetchFromGitHub {
                owner = "open-telemetry";
                repo = "opentelemetry-cpp";
                rev = "refs/tags/v${version}";
                hash = "sha256-Z8YHXLoY0uwSwy60izqNPomfUV6UcTeWV3/eeW8p8dk=";
              };
            })
            abseil-cpp
          ];
          dontUseCmakeConfigure = true;
        };
      };
    };
}
