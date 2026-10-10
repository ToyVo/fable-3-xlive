#!/usr/bin/env bash
# SPDX-License-Identifier: Apache-2.0
# Fetch + install Fable 3 DLC packs for use with the unified xlive.dll.
#
# Usage: ./scripts/fetch-dlc.sh [GAME_DIR]
#   GAME_DIR defaults to the Steam Deck/Linux Steam install, or $GAME_DIR.
#   Override cache/assets: DLC_CACHE_DIR, BASE_PACK_DIR. FORCE=1 to re-extract.
#
# What it does:
#   1. copies assets/90_DLC_xlive_removal/ -> <game>/DLC/90_DLC_xlive_removal/
#   2. downloads the 3 Xbox CDN .cab packs (resumable) into DLC_CACHE_DIR
#   3. extracts each cab root into <game>/DLC/0{1,2,3}_*/
#   4. verifies content.xbx landed in each pack, prints next steps.
set -euo pipefail

GAME_DIR="${1:-${GAME_DIR:-$HOME/.local/share/Steam/steamapps/common/Fable 3}}"
if [[ ! -d "$GAME_DIR" ]]; then
  for alt in "$HOME/.steam/steam/steamapps/common/Fable 3" \
             "$HOME/.var/app/com.valvesoftware.Steam/data/Steam/steamapps/common/Fable 3"; do
    [[ -d "$alt" ]] && GAME_DIR="$alt" && break
  done
fi
if [[ ! -d "$GAME_DIR" ]]; then
  echo "error: game dir not found: $GAME_DIR" >&2
  echo "usage: $0 [GAME_DIR]" >&2
  exit 1
fi

SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(dirname -- "$SCRIPT_DIR")"
BASE_PACK_DIR="${BASE_PACK_DIR:-$REPO_ROOT/assets/90_DLC_xlive_removal}"
DLC_CACHE_DIR="${DLC_CACHE_DIR:-$REPO_ROOT/.dlc-cache}"
DLC_DIR="$GAME_DIR/DLC"
FORCE="${FORCE:-0}"

# pack-dir | cab id | expected bytes | sha256 (empty = size-check only)
PACKS=(
  "01_Understone|4d53090a0ecf0000|256943166|869a0147251b109a8c14ad842e875b381c93c8e6a883a064d9c29b904697caae"
  "02_TraitorsKeep|4d53090a0ecf000e|1025720613|3728c92b4e4ef7f6c78fcf5720a98fcacb960d835d4e5c6d7ac5ff1d4fed9f79"
  "03_InquisitorsPack|4d53090a0ecf0010|1546685|836b94464b01f74e7cad6aeb7ec426b83aac12ecb180da6dfa9508730e136865"
)
CAB_BASE="http://download.xbox.com/content/4d53090a"

have() { command -v "$1" >/dev/null 2>&1; }
if have curl; then DL=(curl -fSL --retry 3 -C - -o); elif have wget; then DL=(wget -c -O); else
  echo "error: need curl or wget" >&2; exit 1
fi

file_size() { stat -f%z "$1" 2>/dev/null || stat -c%s "$1"; }

extract_cab() { # $1=cab $2=dest
  if have cabextract; then cabextract -q -d "$2" "$1"
  elif have 7zz; then 7zz x -y -o"$2" "$1" >/dev/null
  elif have 7z; then 7z x -y -o"$2" "$1" >/dev/null
  elif have bsdtar; then bsdtar -xf "$1" -C "$2"
  else echo "error: need cabextract, 7z or bsdtar to unpack .cab files" >&2; exit 1
  fi
}

mkdir -p "$DLC_DIR" "$DLC_CACHE_DIR"

# 1. base stub pack (the vendored "safe spot"; pass BASE_PACK_DIR to override)
if [[ -d "$DLC_DIR/90_DLC_xlive_removal" && "$FORCE" != "1" ]]; then
  echo "keep: DLC/90_DLC_xlive_removal already present"
elif [[ -d "$BASE_PACK_DIR" ]]; then
  rm -rf "$DLC_DIR/90_DLC_xlive_removal"
  cp -r "$BASE_PACK_DIR" "$DLC_DIR/90_DLC_xlive_removal"
  echo "copy: 90_DLC_xlive_removal/ <- assets/"
else
  echo "warn: base pack not found at $BASE_PACK_DIR; DLC enumeration needs" >&2
  echo "      DLC/90_DLC_xlive_removal/ (see docs/DLC.md)" >&2
fi

# 2-3. download + extract each pack
for spec in "${PACKS[@]}"; do
  IFS='|' read -r dir cab size sha <<<"$spec"
  dest="$DLC_DIR/$dir"; cabfile="$DLC_CACHE_DIR/$cab.cab"
  if [[ -f "$dest/content.xbx" && "$FORCE" != "1" ]]; then
    echo "keep: DLC/$dir already extracted"
    continue
  fi
  echo "== DLC/$dir ($(( size / 1048576 )) MB)"
  if [[ -f "$cabfile" ]] && [[ "$(file_size "$cabfile")" == "$size" ]]; then
    echo "  have: cached cab, size OK"
  else
    echo "  get:  $CAB_BASE/$cab.cab"
    "${DL[@]}" "$cabfile" "$CAB_BASE/$cab.cab"
  fi
  if [[ "$(file_size "$cabfile")" != "$size" ]]; then
    echo "error: size mismatch for $cab.cab (got $(file_size "$cabfile"), want $size)" >&2
    exit 1
  fi
  if [[ -n "$sha" ]]; then
    got="$(sha256sum "$cabfile" 2>/dev/null | cut -d' ' -f1 || shasum -a 256 "$cabfile" | cut -d' ' -f1)"
    [[ "$got" == "$sha" ]] || { echo "error: sha256 mismatch for $cab.cab" >&2; exit 1; }
    echo "  ok:   sha256 verified"
  fi
  mkdir -p "$dest"
  echo "  unpack -> DLC/$dir"
  extract_cab "$cabfile" "$dest"
  [[ -f "$dest/content.xbx" ]] || { echo "error: content.xbx missing after unpack" >&2; exit 1; }
done

echo
echo "done. DLC layout under $DLC_DIR:"
ls -1 "$DLC_DIR"
echo
echo "next: set 'profile xuid 1' in xlive.ini to your save-folder id, then run."
echo "(unified xlive.dll + xlive.ini go next to fable3.exe — see docs/BUILD.md)"
