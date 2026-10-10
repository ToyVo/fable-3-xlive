# `xlive.dll` for Fable 3 (GFWL replacement)

A single clean-room `xlive.dll` that lets Fable 3 run single-player without
Games for Windows – LIVE **and** load DLC packs from a local `DLC\` folder.
It boots like Timeslip's fake xlive and enumerates DLC like the xLiveLess-based
disabler — one DLL, one INI, no dead online services involved.

> Status: source-complete, builds reproducibly with Nix. **Game-tested: no.**
> See [docs/BUILD.md](docs/BUILD.md) for how to build, install, and smoke-test.

## Quick start

**Players: start with [docs/GUIDE.md](docs/GUIDE.md)** — install the release
DLL, fetch the DLC with one script, bind your save. The rest of this file is
for builders.

```sh
# reproducible build (Linux / macOS / WSL — needs Nix with flakes)
nix build .#xlive   # → ./result/xlive.dll + xlive.ini
```

Copy both next to `fable3.exe`, set `profile xuid 1` in `xlive.ini` to your
save-folder id, lay out DLC packs (details in [docs/BUILD.md](docs/BUILD.md)),
run the game. Manual toolchain builds for Windows / Linux / macOS are
documented in [docs/BUILD.md](docs/BUILD.md).

## How it works

- `XLiveInitialize[Ex]` → parses `xlive.ini` (fallback `c:\xlive.ini`),
  4 local profiles, local save store (`save directory`, default `XLiveEmu`).
- Content path (`XContentCreateEnumerator` / `XContentCreatePackage` /
  `XLiveContentGetPath`) scans `DLC\` subdirectories for per-pack
  `content.xbx` — any folder with content is served, so mod-style packs work too.
- Users, storage, sockets: offline success shims backed by the INI + local
  disk; WinSock calls pass through to real WS2_32. Every `XShow*UI` returns
  success silently — no GFWL screens.
- `ShowMessages` (0 ignore / 1 MessageBox / 2 log) and optional trace log
  (`debug log = 1` → `xlive_trace.log`).
- `LoaderPatch` (default **0/off**): opt-in single-byte launcher-check removal
  for the exact 1.1.1.3 exe, size-guarded. Test-only — it provokes SecuROM
  countermeasures (low-detail mode). Leave it off for real play.
- Missing `xlive.ini` but have a Timeslip `fakexlive.ini`? `xuidl`/`xuidh`/
  `profile` are imported automatically as a migration aid.

Details and the full behavior side-by-side that motivated this unification:
[docs/COMPARISON.md](docs/COMPARISON.md). DLC packs (Understone, Traitor's
Keep, Inquisitor's Pack) are fetched and laid out by
`scripts/fetch-dlc.sh` / `scripts/fetch-dlc.bat` — see [docs/DLC.md](docs/DLC.md).

## Layout

```
src/            C sources, xlive.h, xlive.def (249 exports, ordinals pinned),
                xlive.ini (shipped default config), tools/gen_def.py
docs/           BUILD.md (Windows/Linux/macOS), DLC.md, PROVENANCE.md,
                COMPARISON.md
scripts/        fetch-dlc.sh / fetch-dlc.bat (DLC download + layout)
assets/         90_DLC_xlive_removal/ base stub pack (vendored)
flake.nix       nix build .#xlive-unified (i686 Windows cross-compile)
```

## Provenance

Clean-room reconstruction from decompiled behavior — no original source was
available or copied. Full method, reference binaries (hashes), and uncertainty
list: [docs/PROVENANCE.md](docs/PROVENANCE.md). In short: behavior modeled on
Timeslip's fake xlive v0.11 and an xLiveLess 2.0a4-based disabler, analyzed with
Ghidra; export ordinals verified against the real PE tables.

## License

Apache-2.0 — see [LICENSE](LICENSE). SPDX headers mark the covered files.

## Disclaimer

Unofficial, unaffiliated with Microsoft, Lionhead, Timeslip, or xLiveLess
authors. For use with a legally owned copy of Fable 3. This is an
interoperability shim for discontinued online services, not a crack: it
contains no game content, keys, or activation bypasses.
