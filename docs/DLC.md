# DLC setup (automated)

`scripts/fetch-dlc.sh` (Linux/macOS, mainly Steam Deck) and
`scripts/fetch-dlc.bat` (Windows) do guide steps 1–5 for you: base stub pack,
three Xbox CDN downloads, extraction, layout, verification.

```sh
# Deck/Linux (defaults to ~/.local/share/Steam/steamapps/common/Fable 3)
./scripts/fetch-dlc.sh [GAME_DIR]
```

```bat
:: Windows (defaults to %ProgramFiles(x86)%\Steam\steamapps\common\Fable 3)
scripts\fetch-dlc.bat [GAME_DIR]
```

Both are idempotent (skip finished packs) and resumable (big files!), with
`FORCE=1` to redo. Knobs: `GAME_DIR`, `DLC_CACHE_DIR` (default
`<repo>/.dlc-cache`, git-ignored), `BASE_PACK_DIR`.

## What lands where

```
<game>/DLC/
  90_DLC_xlive_removal/   <- vendored from assets/ (the "safe spot")
  01_Understone/          <- 4d53090a0ecf0000.cab, 256,943,166 B
  02_TraitorsKeep/        <- 4d53090a0ecf000e.cab, 382,543,334 B
  03_InquisitorsPack/     <- 4d53090a0ecf0010.cab, 1,546,685 B
```

Each pack extracts to `content.xbx` + `content.cat` + `X_IMAGEID_GAME.png` at
its root and payload banks under `Content/` — exactly what the unified DLL's
content enumerator scans (`docs/COMPARISON.md`).

## Requirements

- `.sh`: `curl` (or `wget`) + one of `cabextract`, `7z`/`7zz`, `bsdtar`
  (Deck: `sudo pacman -S cabextract`; macOS ships `bsdtar`).
- `.bat`: Windows 10+ (`curl.exe` + `expand.exe` are built in; `certutil`
  for hashing).

## Verification

- All packs: exact byte-size **and** sha256 check (table in
  `assets/README.md`). Hashes were recorded from complete CDN downloads on
  2026-10-10; a mismatch means a corrupt/truncated file (or changed upstream)
  — delete the cached `.cab` and rerun, and report it if it persists.
- Post-extract: `content.xbx` must exist per pack or the script fails.

URLs are plain `http://download.xbox.com/…` (Microsoft's CDN, still serving
as of 2026-10-10). If they ever rot, the scripts need new `CAB_BASE`/ids —
please open an issue rather than trusting random mirrors.

## Base pack

`assets/90_DLC_xlive_removal/` is vendored in this repo (~148 KB: stub
`DLC.bnk` + manifests + `content.xbx`) so setup never depends on the old
Dropbox link. Origin: community `DLC_xlive_removal.zip` (see
`assets/README.md`).

## Manual fallback

Download each `.cab` yourself, then per pack: extract the cab root into
`DLC\<pack>\` (Windows: `expand file.cab -F:* DLC\pack\`; Linux/macOS:
`cabextract -d DLC/pack file.cab`), copy the base pack the same way, and
continue at BUILD.md install step 2 (`profile xuid 1`).
