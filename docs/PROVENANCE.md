# Provenance

How this code came to be, what it was modeled on, and what is (and isn't)
certain. If you redistribute or relicense, keep this file with the source.

## Method

Clean-room behavioral reconstruction. Two reference binaries were imported
into Ghidra 12.1.2 and analyzed (auto-analysis + decompiler) via the ReVa MCP
extension; C sources were then written from the observed behavior (config keys
and defaults, control flow, string references, import/export tables). No
original source for either binary was available, and none is reproduced here.

Key Ghidra anchors (program paths are the analyzer's local ones):

- Timeslip config loader `FUN_10009350` @ `0x10009350` (66 lines) → INI keys,
  defaults, clamps; DllMain `FUN_1000a390`; stubs `f001` (`return 0`),
  `f015` (profile-name copy); LoaderPatch block lines 58–64 with
  `ADD ESI,0x21567d` / `MOV byte ptr [ESI],0x0` (`0x100094c8`/`0x100094dc`).
- xLiveLess init `FUN_100012d0` @ `0x100012d0` (151 lines, callers
  `XLiveInitialize` @ `0x10003fd0` / `XLiveInitializeEx` @ `0x10005430`);
  DLC enumerator `FUN_10002550` @ `0x10002550` (218 lines, callers
  `XContentCreateEnumerator`, `XContentCreatePackage`, `XLiveContentGetPath`).
- Export ordinals were **not** taken from Ghidra names but verified with
  `pefile` against both PE export tables (`src/tools/gen_def.py`, which also
  generates `src/xlive.def`): 249 named exports at true ordinals; every
  Timeslip `f<N>` ordinal coincides with one of them.

## Reference binaries (analyzed, not shipped)

- `fable3-xlive-zip/xlive.dll` == `fable3-xlive-7z/xlive.dll`
  sha256 `7dd7b0a22399fcd62d7f5c5427dc2de530b7868f46680b43adf5a6b4b69f9038`,
  95,232 bytes — Timeslip "Fake xlive.dll for Fable 3" **v0.11** (per its
  bundled `readme.txt`; single-player GFWL stub, free-DLC experiment,
  test-only LoaderPatch).
- `gfwl-disabler/xlive.dll`
  sha256 `136c2dbae15022efa9a791b2099fbee2aaf2b090f13e172ee39043a350646a54`,
  215,040 bytes — xLiveLess **2.0a4**-based disabler (per embedded
  `Log started (xLiveLess 2.0a4)`); INI profiles, save store, DLC/content.xbx
  enumeration, full XLive surface.

Neither binary, nor any game asset, save, archive, or key, is included in this
repository. The per-binary reconstructions used during analysis are not
published either; only this unified implementation is.

## Inspirations and prior art

- Timeslip's fake xlive (Fable 3 v0.11): config schema (`fakexlive.ini`
  `[xlive]` keys, defaults, clamps), `ShowMessages`/`UseDLC`/`LoaderPatch`
  semantics, ordinal-stub approach. Distribution: fan mod archives/readmes.
- xLiveLess 2.0a4 lineage: `xlive.ini` schema (`profile name/xuid N`, `signin
  player N`, `online profile`, `save directory`, `debug log`), `content.xbx`
  DLC enumeration, named XLive export surface with true ordinals.
- Microsoft XLive API shapes are used nominally (function names/ordinals for
  binding compatibility); implementations are original shims.

## Known uncertainties

- Uncalled-by-the-game stubs share one `StubSuccess` callee; stdcall arity on
  those paths is binding surface, not verified behavior (same posture as
  xLiveLess's own `XLiveLess_5xxx` placeholders).
- LoaderPatch target (`Fable3.exe + 0x21567D`, single zero byte) is inferred
  from disassembly + decompiler agreement and guarded by exact exe-size check;
  verify with a debugger before enabling.
- `content.xbx` binary parse is represented as folder-presence + ContentID
  trace; byte-level format details were not fully recovered.
- 16-digit save XUIDs exceed 32 bits; only the low DWORD is stored (matches
  observed slots — confirm against your save id).
