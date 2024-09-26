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
            pkg-config
          ];
          buildInputs = [ fuse3 ];
        };
      };
    };
}
