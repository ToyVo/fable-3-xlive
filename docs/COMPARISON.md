# What the two xlive.dlls do — comparison

Your hash observation is the whole story in one line: the two download sources
for the "Windows vs Steam Deck" sections are the **same binary** (Timeslip
v0.11), and the DLC-enabling step swaps in a **different, larger binary**
(xLiveLess 2.0a4-based). (Export counts below are Ghidra's view: `entry` counted once and
one export missed — PE ground truth is 102 Timeslip / 249 xLiveLess exports.)

## At a glance

| | Timeslip fake xlive v0.11 (`7dd7b0a2…`, 95 KB) | xLiveLess disabler (`136c2dba…`, 215 KB) |
|---|---|---|
| Identity | `readme.txt`: "Fake xlive.dll for fable 3, by Timeslip v0.11" | Embedded `Log started (xLiveLess 2.0a4)` (`0x1001ff28`) |
| Size/sections | 4 sections, `.text` 51 KB, `.data` 14 KB | 5 sections (+`.rsrc`), `.text` 118 KB, `.data` **634 KB** (profile/DLC tables) |
| Functions/exports/strings | 307 / **103** / 297 | 616 / **248** / 992 |
| Export style | **Ordinal shims** `f001…f5376` (names stripped; game binds by ordinal) | **Named XLive API** (`XLiveInitialize`, `XUser*`, `XSocket*`, `XContent*`, `XShow*UI`, …) + `XLiveLess_5xxx` internals |
| Imports | KERNEL32:71, D3DX9_42:3, SHELL32/SHLWAPI/USER32:1 each | KERNEL32:99, USER32:11, **WS2_32:12**, WINTRUST:1 (real sockets, no D3D) |
| Config file | `%APPDATA%\Lionhead Studios\Fable 3\**fakexlive.ini**` (`0x1000fa34`), `[xlive]` section | `**xlive.ini**` next to exe, fallback `c:\xlive.ini` (`0x1001fdb0`/`0x1001fdbc`) |
| Identity model | Single offline profile: `xuidl/xuidh` (default `0x10001000` each), `profile` (default `Player`, ≤32 chars) | **4 profiles**: `profile name N`, `profile xuid N`, `signin player N`, `online profile`, `save directory` (default `XLiveEmu`) |
| DLC story | `UseDLC` 0/1/2 (default 0): 1 → `data\dlc\content` (free DLC, 40 files), 2 → `data\mod0..9` (mods-as-DLC). **Paid DLC unsupported** (readme). Experimental, save becomes DLC-dependent | **Real DLC enumeration**: `FindFirstFileW` over `DLC\` subdirs, per-folder `\content.xbx`, traces `- DLC%d = %s`, `=== ContentID = %s` (`FUN_10002550`). This is why the guide's `01_Understone/02_TraitorsKeep/03_InquisitorsPack` layout only works with this DLL |
| Messages/UI | `ShowMessages` 0/1/2 (default 2): ignore / MessageBox / **in-game D3D text** (D3DXCreateFontW/Sprite) | All `XShow*UI` return success silently; optional `xlive_trace.log` (`debug log = 1`) |
| Networking | None (single-player only; "not compatible with multiplayer") | Thin WinSock wrappers (`XSocket*`, `XNet*`, `XWSA*`) so network-adjacent calls don't crash; no real LIVE connection |
| Saves | XUID-gated: save's remote-XUID must match `xuidl/xuidh` (or edit save to `1000100010001000`); checksums via save editor | `save directory = XLiveEmu` local store; `profile xuid 1 = <save-folder-id>` (guide step 11) binds saves to profile 1 |
| Exe patching | `LoaderPatch` 0/1: single-byte `VirtualProtect` patch removing launcher check; trips SecuROM low-detail mode; test-only | None observed |
| DllMain | `DisableThreadLibraryCalls` + `LoadConfig`, return 1 (`FUN_1000a390`) | `XLiveInitialize[Ex]` → one-shot init + critical section + trace line, return 0 (`FUN_100012d0`) |

## Why DLC needs the swap

Fable 3 discovers DLC through the XLive **content-enumeration** path
(`XContentCreateEnumerator` → `XLiveContentGetPath` → `XEnumerate`). The
Timeslip DLL answers those ordinals with canned success/empty (and its readme
says paid DLC is unsupported), so `DLC/01_*…` folders are invisible. The
xLiveLess DLL actually **scans `DLC\` and parses per-pack `content.xbx`**,
which is exactly the layout `guide.md` builds. Same reason saves need the
`profile xuid 1` edit: the disabler keys saves off INI profiles, not the GFWL
account.

## Can you trust them?

Honestly: **no more than any unsigned binary**, and these are worse than
average (no source, no signatures, game-adjacent forum/Dropbox provenance,
and they run in-process with full user privileges). What this reconstruction
gives you instead of blind trust:

- The observed behavior is **mundane**: INI parsing, stub-success returns,
  local DLC/save emulation, WinSock pass-through. Nothing in strings, imports,
  or traced functions suggests exfiltration (no `HttpSendRequest`, no suspicious
  hosts; imports are KERNEL32/USER32/WS2_32/WINTRUST + D3DX for the overlay).
  Absence of evidence ≠ proof — a targeted backdoor wouldn't need new imports
  (`CreateThread` + sockets are already there in the disabler).
- Concrete verification steps if you want more assurance:
  1. Rebuild the reconstructions and **diff behavior** (same exports, same INI
     keys, same DLC scan) rather than running the binaries.
  2. Run strings/imports diffs yourself (`src/xlive.def`, `src/xlive.ini`).
  3. Network-monitor a play session (expect: no outbound LIVE traffic from
     either DLL; Timeslip makes none at all by construction).
  4. For the paranoid path: keep the Timeslip DLL for base game, swap in the
     disabler only for DLC sessions, from a limited user account.

## Ghidra map (to check my work)

- Timeslip config: `FUN_10009350` (`0x10009350`), DllMain `FUN_1000a390`,
  stubs `f001` (`0x10009340`, `return 0`), `f015` (`0x10009250`, profile copy),
  INI path `0x1000fa34`, keys `xuidl/xuidh/profile/ShowMessages/UseDLC/
  LoaderPatch`, `GetPrivateProfileIntA/StringA` imports.
- Disabler init: `FUN_100012d0` (`0x100012d0`, 151 lines; callers
  `XLiveInitialize` `0x10003fd0` / `XLiveInitializeEx` `0x10005430`), INI keys
  `profile name/xuid 1-4`, `online profile`, `save directory`, `debug log`,
  banner `0x1001ff28`.
- Disabler DLC: `FUN_10002550` (`0x10002550`, 218 lines; callers
  `XContentCreateEnumerator`, `XContentCreatePackage`, `XLiveContentGetPath`),
  `DLC\` (`0x1001ffd8`), `\content.xbx` (`0x1001ffe4`), `- DLC%d = %s`,
  `=== ContentID = %s`, `- [%d] DLC = %08X` (in `XEnumerate`).
- Users: `XUserGetName` (`0x10004cb0`), `XUserGetSigninInfo` (`0x10004e30`).
