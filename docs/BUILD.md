# Building the `xlive.dll`

Target is always **32-bit Windows** (`i686` / PE32 DLL), no matter which OS you
build on. Any build produces two files: `xlive.dll` + `xlive.ini`.

## Option A — Nix (recommended, all OSes incl. macOS and WSL)

Needs [Nix](https://nixos.org/download) with flakes enabled.

```sh
nix build .#xlive
ls ./result/   # xlive.dll  xlive.ini
```

This cross-compiles with pinned `i686-w64-mingw32-gcc` — bit-identical
toolchain for everyone, no local setup. (`nix develop` drops you in a shell
with the same toolchain plus `uv`/`python3` for the export-check script.)

## Option B — Windows, MSVC (x86 Native Tools prompt)

Needs Visual Studio Build Tools with the MSVC x86 toolchain:

```bat
cd src
cl /LD /O2 /W3 /WX /std:clatest /D_CRT_SECURE_NO_WARNINGS /Fe:xlive.dll *.c xlive.def ws2_32.lib shell32.lib user32.lib
```

(`/std:clatest` for mixed declarations, `/WX` so warnings fail the build,
`_CRT_SECURE_NO_WARNINGS` to silence MSVC's `fopen`/`sscanf` deprecation
noise. `winsock2.h` is included before `windows.h` in `xlive.h` — reversing
that is a hard error on MSVC.)

## Option C — Windows, MinGW (MSYS2 MINGW32 shell)

```sh
pacman -S mingw-w64-i686-toolchain
cd src
i686-w64-mingw32-gcc -shared -O2 -Wall -o xlive.dll *.c xlive.def \
  -lws2_32 -lshell32 -luser32 -Wl,--enable-stdcall-fixup -Wl,--kill-at
```

## Option D — Linux (native mingw-w64)

Debian/Ubuntu:

```sh
sudo apt install gcc-mingw-w64-i686
```

Fedora:

```sh
sudo dnf install mingw32-gcc mingw32-headers
```

Then the same `i686-w64-mingw32-gcc …` command as Option C from `src/`.

## Option E — macOS without Nix

No supported recipe yet: Homebrew's `mingw-w64` is 64-bit-only and this DLL
must be 32-bit. Use Option A. (An `llvm-mingw` i686 recipe may work and would
be a welcome PR.)

## Versioning and releases

The version lives in exactly one place: `src/version.h`
(`XLIVE_UNIFIED_VERSION`). It is compiled into the DLL's `xlive_trace.log`
header (`Log started (unified v…)`), read by `flake.nix`
for the nix package version, and used by CI to tag releases. To cut one:

1. Bump the string in `src/version.h`, commit, push.
2. Actions → `release` → Run workflow (no inputs).

The workflow reads the version, and if release `v<version>` already exists it
succeeds quietly doing nothing. Otherwise it rebuilds `xlive.dll` with MSVC
x86 on `windows-latest`, verifies exports, creates tag `v<version>`
automatically, and attaches `xlive.dll`, `xlive.ini`, `fetch-dlc.bat`, and
`xlive.dll.sha256` to the GitHub Release. Every push/PR also builds both
toolchains in `.github/workflows/build.yml` (MSVC x86 + mingw cross) as CI
artifacts without releasing.

## Verify the build

Expect **249 named exports** with pinned ordinals (spot-check: `1 XWSAStartup`,
`5000 XLiveInitialize`, `5297 XLiveInitializeEx`, `5263 XUserGetName`,
`5355 XLiveContentGetPath`):

- Windows: `dumpbin /exports xlive.dll` (VS prompt).
- Any OS: `uv run --with pefile python -c "`…`"` or `llvm-objdump`:

```sh
uv run --with pefile python - <<'EOF'
import pefile, sys
pe = pefile.PE("xlive.dll")
ex = pe.DIRECTORY_ENTRY_EXPORT.symbols
print(len(ex), "exports")
want = {1: "XWSAStartup", 5000: "XLiveInitialize", 5297: "XLiveInitializeEx"}
got = {e.ordinal: e.name.decode() for e in ex}
print({k: got.get(k) for k in want})
EOF
```

## Install & smoke-test (on the Windows machine / Deck)

1. Copy `xlive.dll` + `xlive.ini` next to `fable3.exe`.
2. Open your Fable 3 save folder, copy its folder id (e.g. `1000100010001000`),
   set `profile xuid 1 = <that id>` in `xlive.ini`.
3. DLC packs: one folder per pack under `<game>\DLC\` (e.g. `01_Understone`,
   `02_TraitorsKeep`, `03_InquisitorsPack`), each with its content payload.
4. Run. Checklist: boots to menu · no GFWL screens · save loads · Sanctuary
   gifts (DLC) present · `debug log = 1` produces `%TEMP%\xlive_trace.log`
   if you need diagnostics. No outbound LIVE traffic is expected.
