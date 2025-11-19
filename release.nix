let
  pkgs = import <nixpkgs> {};
in
pkgs.callPackage ./default.nix {
  src = pkgs.lib.cleanSource ./.;
}
