{ pkgs ? import <nixpkgs> {} }:
with pkgs;
stdenv.mkDerivation {
  pname = "datefmt";
  version = "0.1.1";

  makeFlags = "PREFIX=$(out)";

  src = ./.;
}
