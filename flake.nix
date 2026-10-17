{
  description = "Fable 3 patch - Ghidra + ReVa MCP reverse-engineering env";
  # NOTE (Windows): Nix has no native Windows port, so this flake only
  # covers Linux/macOS (/WSL). For native Windows, run
  # scripts/setup-ghidra-mcp-windows.ps1 — see docs/GHIDRA.md.

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";
  };

  outputs =
    { self, nixpkgs }:
    let
      systems = [
        "aarch64-darwin"
        "aarch64-linux"
        "x86_64-linux"
      ];
      forEachSystem = nixpkgs.lib.genAttrs systems;
    in
    {
      packages = forEachSystem (
        system:
        let
          pkgs = import nixpkgs {
            inherit system;
            config.allowUnfree = true;
          };
          ghidra-reva = pkgs.ghidra.withExtensions (p: [ p.reva ]);
          # Version is owned by src/version.h (XLIVE_UNIFIED_VERSION) — the
          # same header compiled into the DLL log banner. Bump it there.
          unifiedVersion =
            let
              lines = builtins.filter builtins.isString (
                builtins.split "\n" (builtins.readFile ./src/version.h)
              );
              pick =
                acc: line:
                let
                  m = builtins.match "#define XLIVE_UNIFIED_VERSION \"([0-9]+\\.[0-9]+\\.[0-9]+)\"" line;
                in
                if acc != null then acc else if m == null then null else builtins.head m;
              found = builtins.foldl' pick null lines;
            in
            if found == null then
              throw "could not parse XLIVE_UNIFIED_VERSION from src/version.h"
            else
              found;
          # xlive.dll, cross-compiled for 32-bit Windows with mingw.
          # `nix build .#xlive` → $out/xlive.dll + xlive.ini
          xlive = pkgs.pkgsCross.mingw32.stdenv.mkDerivation {
            pname = "xlive";
            version = unifiedVersion;
            src = ./src;
            dontConfigure = true;
            buildPhase = ''
              runHook preBuild
              $CC -shared -O2 -Wall -o xlive.dll *.c xlive.def \
                -lws2_32 -lshell32 -luser32 -static-libgcc \
                -Wl,--enable-stdcall-fixup -Wl,--kill-at
              runHook postBuild
            '';
            installPhase = ''
              runHook preInstall
              mkdir -p $out
              cp xlive.dll xlive.ini $out/
              runHook postInstall
            '';
          };
        in
        {
          inherit ghidra-reva xlive;
          ghidra = ghidra-reva;
          default = ghidra-reva;
        }
      );

      devShells = forEachSystem (
        system:
        let
          pkgs = import nixpkgs {
            inherit system;
            config.allowUnfree = true;
          };
        in
        {
          default = pkgs.mkShell {
            packages = [
              pkgs.uv
              pkgs.python3
            ];

            shellHook = ''
              echo "— xlive build env —"
              echo "  nix build .#xlive   → ./result/xlive.dll + xlive.ini"
              echo "  manual builds: see docs/BUILD.md"
            '';
          };
        }
      );
    };
}
