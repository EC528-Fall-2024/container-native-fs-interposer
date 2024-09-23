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
          contents = [ csi ];
          config.Entrypoint = [ "/bin/csi-node" ];
        };
      };
    };
}