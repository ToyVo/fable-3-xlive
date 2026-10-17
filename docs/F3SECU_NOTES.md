# F3Secu.exe — Launch / Verification Flow Notes (SecuROM 7, Fable 3 Steam 1.1.1.3)

> Status: **documentation only**. The on-disk binary was never modified. All
> addresses are given as **absolute (image base 0x400000)** plus **RVA**.
> File offsets (FO) are 0-based offsets into `F3Secu.exe` on disk.
> Claims are tagged **[OBSERVED]** (REA/tool output), **[INFERRED]** (analyst
> conclusion from observations), or **[OPEN]** (unresolved — not guessed).
> SHA-256 of analyzed file: `9543779efe38588f417a3d6ab1986ff8d58f7792e8a02557d565d05ac73a8d98`
> REA provider: Ghidra 12.1.2 (`binary_overview` ev_c374d667…).

Chain (corroborated): `FableLauncher.exe` contains the ASCII ref `F3Secu.exe`
and PDB `F:\dev\fable3\PCBranch\SourceCode\Tools\FableLauncher\Release\FableLauncher_SecuROM.pdb`
**[OBSERVED]** (local `tr`/`grep` of FableLauncher.exe). So:
`FableLauncher.exe -> F3Secu.exe -> Fable3.exe`.

## 0. Binary layout (already-established facts, re-verified)

- PE32 i386, image base 0x400000 **[OBSERVED]** (`Headers` segment starts
  0x400000, ev_c374d667…).
- Entry procedure `entry` @ **abs 0xE23880 / RVA 0xA23880 / FO 0x51BA80**
  **[OBSERVED]** (list_procedures ev_2c4e1754…; address_to_file_offset
  ev_c1d85651… → FO 5355648 = 0x51BA80).
- `Stext` abs 0x418000–0xE1F5D0 (= RVA 0x18000, ~10.5 MB), `.securom` abs
  0x11FA000–0x1380000 (= RVA 0xDFA000, ~1.6 MB) **[OBSERVED]** (ev_c374d667…).
- 350 procedures = **334 EXTERNAL imports + 16 internal** (6 in Stext, 10 in
  Sitext incl. `entry`) **[OBSERVED]** (ev_2c4e1754…). The 6 Stext "functions"
  are junk: e.g. `FUN_006a5020` is 9 bytes (`LDS EDI,[ECX]; INC ESI; JMP
  0xeb9531a6` → wild jump, decompiler: "bad instruction data")
  **[OBSERVED]** (ev_e875c88a…).
- 10 import DLLs: USER32 66, KERNEL32 196, CRYPT32 5, ADVAPI32 36, COMCTL32 2,
  SHLWAPI 10, OLE32 3, SHELL32 7, WSOCK32 6, VERSION 3 **[OBSERVED]**
  (ev_2c4e1754…).
- Wide string `Fable3.exe` @ **abs 0x11FE414 / RVA 0xDFE414 / FO 0x6CC414**,
  UTF-16LE bytes `46 00 61 00 … 65 00 65 00` **[OBSERVED]** (search_strings
  ev_299f49d6…; read_bytes ev_ddae0427…; FO ev_645b13fe… → 7128084).
- `F3Secu.exe.ini` is UTF-16, `TITLEID=4D53090A`, `TITLE_NAME=Fable III`,
  blocks DEFAULT/DE-DE/FR-FR/ES-ES/IT-IT/NL-NL/SV-SE **[OBSERVED]** (local read).

## 1. API-redirection table identity

The `.securom` table at abs ~0x1202800–0x12032FF holds 4-byte RVAs pointing at
`[prefix0][prefix1]<ASCII name>\0` records **[OBSERVED]** (tabA ev_53346f5b…,
tabB ev_85052ed0…). Record format detail **[INFERRED, low confidence]**:
second prefix byte looks like a DLL group (0x00/0x01/0x02).

### 1.1 The four asked-about slots — none is CreateProcess

| Slot (abs / RVA / FO) | On-disk dword → record | Record bytes → API name | Evidence |
|---|---|---|---|
| 0x12028C5 / 0xE028C5 / 0x6D08C5 | `65 15 E0 00` = RVA 0xE01565 | `77 01` + `GetModuleHandleA` | ev_18277eb0…, ev_a4f6cb30…, ev_1dbc6600… |
| 0x1202969 / 0xE02969 / FO = slot+0xA4 | `46 18 E0 00` = RVA 0xE01846 | `0C 02` + `HeapFree` | ev_d92d3b47…, ev_7bb367c7… |
| 0x120296D / 0xE0296D | `51 18 E0 00` = RVA 0xE01851 | `06 02` + `HeapAlloc` | ev_2fd7983c…, ev_83ba559e…, ev_f124b567… |
| 0x12029A1 / 0xE029A1 | `22 19 E0 00` = RVA 0xE01922 | `D5 01` + `GetTickCount` | ev_d607368c…, ev_c30f3e77…, ev_7796d67d… |

So the four slots are **early-bootstrap APIs** (module handle, heap,
tick-count — the classic SecuROM anti-dump/anti-debug set per the ARTeam
dissection) **[INFERRED]**. No data-xref to any of the four slots exists in
Ghidra **[OBSERVED]** (xrefs ev_668e7e20…, ev_f5aa782d…, ev_1cc9b308…,
ev_16bf4390… all empty) — the `FF15` users are hand-spliced code Ghidra does
not decode (see §2).

### 1.2 CreateProcess slots (the real launch slots)

- Name records: `CreateProcessW` @ abs **0x1201C45**, `CreateProcessA` @ abs
  **0x1201D64** **[OBSERVED]** (ev_93360fad…; bytes ev_1c3341f6…,
  ev_de968bda…). No import-table entry for either exists (only these
  `.securom` records) **[OBSERVED]** (search found no 0x40Cxxx hit).
- Record starts (2-byte prefix included): W = abs **0x1201C43**
  (`63 00` + name) **[OBSERVED]** (ev_fdde090a…, ev_3055fc12…);
  A = abs **0x1201D62** (`60 00` + name) **[OBSERVED]** (ev_9241ee2b…).
- Slots holding RVA 0xE01C43 (`43 1C E0 00`): **abs 0x1202A59
  (FO 0x6D0A59)** and dup **0x1202FB9** **[OBSERVED]** (table scan
  ev_dd835b89…; slot bytes ev_383a6678… = `…43 1C E0 00…`; FO ev_4748dd2a…).
- Slots holding RVA 0xE01D62 (`62 1D E0 00`): **abs 0x1202A99** and dup
  **0x1202FF9** **[OBSERVED]** (ev_c11fb129…).
- The table is duplicated with a **0x560 stride** (0x1202FB9−0x1202A59 =
  0x560; GetTickCount slot dup 0x1202F01−0x12029A1 = 0x560) **[OBSERVED]**.
- **Which one launches:** `CreateProcessW` via slot **0x1202A59** (primary;
  dup 0x1202FB9) **[INFERRED]** — the child image name on disk is the *wide*
  string `Fable3.exe`, and a `GetStartupInfoW` record exists at abs 0x1201C33
  **[OBSERVED]** (ev_4f3f1a6c…), so the `STARTUPINFO` setup is Unicode.
  `CreateProcessA` (slots 0x1202A99/0x1202FF9) is the fallback **[INFERRED]**.
- Runtime fill **[INFERRED]**: on disk each slot holds a name-record RVA; at
  runtime the loader overwrites slots with real API addresses (task premise +
  dissection). Mechanism evidence: direct imports of `GetProcAddress` and
  `LoadLibraryA` exist (KERNEL32 externals) **[OBSERVED]** (ev_2c4e1754…),
  `GetProcAddress` name records also sit in `.securom` (abs 0x12014AF) and
  Sitext (0xE2D0F4) **[OBSERVED]** (ev_cc6d33a1…), and Sitext holds
  table-building decoders `FUN_00e252f0` (bit-permutation init over tables
  0x11F92B8→0x11F77C0…, ev_b997f6e2…) and `FUN_00e253a0` (0x200-dword zeroing
  + key-expansion into bitmap via tables 0x11F92D8/0x11F92F8, ev_c10e7fcd…).
  The exact fill loop that calls `GetProcAddress` per record is VM-wrapped
  **[OPEN]**.

## 2. Live vs decoy (0x505000 splice pair)

- **LIVE = high copy: abs 0xE2070F / RVA 0xA2070F.** Bytes
  `FF 15 C5 28 20 01 …` = `CALL DWORD PTR [0x012028C5]` (slot #1,
  GetModuleHandleA), followed by `8B F8` (`MOV EDI,EAX`) +
  `66 81 3F 4D 5A` (`CMP WORD PTR [EDI], 'MZ'`) **[OBSERVED]**
  (ev_f4a17342…, ev_85067929…, ev_70d6fab…). FO of the call = 5342991
  (0x51870F) **[OBSERVED]** (ev_6d4c30fc…). 1 KB scan 0xE20500–0xE20900 finds
  exactly one `FF15` — this one **[OBSERVED]** (ev_fb8de575…).
- **DECOY = low copy: abs 0x91B70F / RVA 0x51B70F.** 24 bytes read as all
  zeros; Ghidra: `undecodable` **[OBSERVED]** (ev_0a14f736…, ev_edc489a7…).
- Corroboration **[INFERRED]**: the entry point (RVA 0xA23880) sits ~13 KB
  above the live call site in the same high region; the low region has no
  recovered code at all. Ghidra decodes nothing at either spliced address
  (`undecodable`, resolve_native_call_targets → `unavailable`, ev_7a553d85…),
  i.e. static disassembly of the live path stops at the raw bytes quoted above.
- **Do not patch anything near 0x91B70F/RVA 0x51B70F** — it is zero fill.

## 3. Launch sequence (process start → CreateProcess of Fable3.exe)

Ordered; each step tagged.

0. `FableLauncher.exe` spawns `F3Secu.exe` **[OBSERVED]** (launcher embeds
   `F3Secu.exe`, §0; child target `Fable3.exe` wide string in F3Secu).
1. **Entry dispatcher** @ 0xE23880: `PUSHFD; PUSHFD; SUB ESP,0x14`, stack-cookie
   loop (`MOV EAX,0xE23850 … JNZ 0xE238AA`), flag-dependent `JNC`, and
   return-address tampering (`ADD [ESP],imm32 … JMP [ESP]-style` tails)
   **[OBSERVED]** (assembly ev_eb3f73c1…, 143 bytes / 6 blocks / no callees,
   no strings, ev_93e610ab…). **[INFERRED]**: VM-trampoline prologue, cf.
   dissection "Virtual Machine" chapter.
2. **Table decoders run** (`FUN_00e252f0`, `FUN_00e253a0`, Sitext)
   **[INFERRED order]** — they build the API-redirection/import tables that
   every later step depends on; both are clean, call-free, table-driven
   routines **[OBSERVED]** (ev_b997f6e2…, ev_c10e7fcd…).
3. **Image self-check**: `CALL [GetModuleHandleA-slot]` → `MOV EDI,EAX` →
   `CMP WORD [EDI],'MZ'` @ 0xE2070F–0xE2071C **[OBSERVED]** (§2 bytes).
4. **License/registry checks** **[INFERRED, components OBSERVED]**:
   - Full registry API set directly imported: `RegOpenKeyExA`, `RegOpenKeyA`,
     `RegQueryValueExA`, `RegEnumKey/Value*`, `RegCreateKeyExA`, … (ADVAPI32)
     **[OBSERVED]** (ev_2c4e1754…).
   - Key/material strings: `Software\SecuROM`, `…\License information - Do not
     delete!`, `…\UserData`, `…\UserData\Backup`, `securom_v7_01.dat/.bak/.tmp`,
     `SecuROM`, `TITLEID`-adjacent ini **[OBSERVED]** (ev_d2d6574a…;
     ev_26c9d807…).
   - Build provenance: `E:\hudson\jobs\SecuROM_Releasebuild\workspace\SecuROM-DRM_8_03\SecuROM_DRM\Wrapper\wrapper.pdb`
     **[OBSERVED]** (ev_d2d6574a…).
5. **Disk/optical checks** **[INFERRED, names OBSERVED]**: `DeviceIoControl`
   (0x1201D75), `GetDriveTypeA` (0x1201DCB), `CreateFileA/W`, `CreateFileMappingA`
   records in `.securom` (ev_53a9294e…, ev_b6d6f09a…, ev_44924e9d…); none are
   direct imports, so they resolve through the redirection table. Exact
   ioctls/order are VM-internal **[OPEN]**.
6. **Activation check** **[INFERRED, strings OBSERVED]**: `activation failed`
   (0xE41784), `server error - …` family, `PA_REQUEST_SET/GET_ACTIVATION_SERVERS`,
   `ProductActivationSetup.exe`, `Content Activation` (ev_0acbfaae…); WSOCK32
   (6) + CRYPT32 (5) imports (ev_2c4e1754…). Server round-trip content/order
   **[OPEN]** (no xrefs: ev_f620f596… empty — VM-referenced).
7. **Service + IPC setup** **[INFERRED, pieces OBSERVED]**:
   `SecuROM User Access Service (V7)` (0xE3F30C), `CreateServiceA/
   StartServiceA/OpenServiceA/ControlService/…` imports, `\.\\pipe\SecuROM_Profiler`,
   `pipe_entries_mutex`, `pipe_init_mutex`, `CreateMutexA/ReleaseMutex/
   OpenMutexA` records+imports (ev_d2d6574a…, ev_619d776e…, ev_2c4e1754…).
8. **Child launch** **[INFERRED sequence, endpoints OBSERVED]**:
   `GetStartupInfoW` → `CreateProcessW` (slot 0x1202A59) with wide
   `Fable3.exe` (0x11FE414) → `ResumeThread` → `WaitForSingleObject` →
   `GetExitCodeProcess` → `TerminateProcess`/`ExitProcess`. All post-CreateProcess
   APIs are direct KERNEL32 imports **[OBSERVED]** (ev_2c4e1754…); the
   CreateProcess-side records are redirection-only (§1.2). Suspended-vs-normal
   creation flag and argument block are VM-built **[OPEN]** (no static xref to
   0x11FE414: ev_270060aa… empty).

## 4. Decision point(s)

### DP-1 (concrete, statically observed): MZ-validation branch

```
abs 0xE2070F / RVA 0xA2070F / FO 0x51870F : FF 15 C5 28 20 01   CALL [0x12028C5]  (GetModuleHandleA slot)
abs 0xE20715 : 8B F8          MOV EDI,EAX
abs 0xE20717 : 66 81 3F 4D 5A CMP WORD PTR [EDI],0x5A4D ('MZ')
abs 0xE2071C : 9C             PUSHFD            (preserve CMP flags)
abs 0xE2071D : 68 B1 29 00 00 PUSH 0x29B1
abs 0xE20722 : 75 16          JNZ -> 0xE2073A  (taken, FO 0x51873A)
                              fall-through 0xE20724 (FO 0x518724)
```
Bytes **[OBSERVED]** (ev_85067929…, ev_70d6fab…, ev_b245c44b…,
ev_988fbbb5…, ev_e92ccbc6…; FOs ev_54f49e17…, ev_37bcae81…).
Ghidra cannot decode any of these as instructions (`undecodable`) and reports
no call target (`unavailable`) **[OBSERVED]** (ev_43a33b02…, ev_623c7c7a…,
ev_7a553d85…) — manual decode above is from raw bytes.
Condition **[INFERRED, standard x86 semantics]**: JNZ tests ZF from the CMP —
taken = memory word ≠ 'MZ' (presumed *failure* path to 0xE2073A),
fall-through = 'MZ' present (presumed *continue* path to 0xE20724). Both
targets begin with `ADD [ESP],imm32` return-address/decoder tails
(`81 04 24 DE 71 DB 00 …` vs `81 04 24 09 DE E1 32 …`) **[OBSERVED]**.
Polarity note: taken=fail is convention + dissection-consistent but the tails
are opaque, so polarity is **[INFERRED, medium confidence]** — verify
dynamically before editing (see §7).

### DP-2 (observed branch, NOT a pass/fail gate): entry dispatcher `JNC`

`0xE238E2: 73 13  JNC 0xE238F7` (fall-through 0xE238E4 does
`ADD [ESP],0xF3E22759`; target does `ADD [ESP],0xA3E224D7`) **[OBSERVED]**
(bytes ev_6be26888…, decoded ins ev_1e79da4b…, FO 0x51B8E2).
**[INFERRED]**: carry-flag VM dispatcher, not a license decision — do not use
as an edit point.

### The ultimate "checks passed → CreateProcess" branch: **[OPEN]**

No static branch into a `CreateProcess` call site was recoverable: no xrefs to
the CreateProcess records (ev_c97202ae…, ev_07639bfb… empty), to `Fable3.exe`
(ev_270060aa… empty), or to any activation/registry/service string
(ev_c9ce8c2e…, ev_d5641b56…, ev_f620f596… empty); only 16 internal functions
were recovered and the launch logic is VM-wrapped (§0). The pass/fail gate
lives inside the VM **[INFERRED from absence + dissection VM chapter]**.

## 5. Post-launch monitoring

- **DFA module on disk**: `DFA.dll`, 6,252,000 bytes, same timestamp as
  F3Secu — version resource `Content Activation Library, FileVersion 2.4.0,
  ProductName Content Activation`, plus embedded `DFAActivationSetup`,
  `Content Activation Setup`, `PeekNamedPipe`, `WS2_32.dll` refs
  **[OBSERVED]** (local `tr`/`grep` + `ls -l`). **[INFERRED]**: SecuROM DFA
  (file-authentication) client — the "memory-vs-disk" checker (per ARTeam
  "CRC checks" chapter: memory/register CRC flavours over mapped image).
- **Wait/exit-code loop in F3Secu**: direct imports `WaitForSingleObject`,
  `GetExitCodeProcess`, `TerminateProcess`, `ResumeThread`, `CreateEventA/
  ResetEvent`, `ConnectNamedPipe` **[OBSERVED]** (ev_2c4e1754…, ev_f41146a4…,
  ev_787b5e26…). **[INFERRED]**: parent stays resident, waits on the child,
  enforces exit policy.
- **Service + pipe presence**: `SecuROM User Access Service (V7)`,
  `CreateServiceA/StartServiceA`, `SecuROM_Profiler` pipe + mutexes (§3.7)
  **[OBSERVED]**. **[INFERRED]**: kernel/service-assisted watchdog path.
- Exact DFA↔F3Secu handoff (which thread, cadence, what is compared)
  **[OPEN]** — no `DFA` string inside F3Secu (ev_c6aaac66…: 0 hits); the
  binding is VM-computed or via the child.

## 6. Candidate byte-edit table (NOT applied — enumeration only)

Conventions: FO = file offset (hex). "Current" bytes verified by `read_bytes`;
FO verified by `address_to_file_offset`. All edits assume a backup copy first.

| # | Address (abs / RVA / FO) | Current → Proposed | Rationale | Risk |
|---|---|---|---|---|
| 1 (primary) | 0xE20722 / 0xA20722 / **0x518722** | `75 16` → `90 90` (JNZ→NOPs) | Force MZ-check fall-through (presumed success-continue path to 0xE20724) past image validation; 2 bytes, no layout change | **Medium**: polarity is inferred (§4 DP-1); per dissection, nearby opcode bytes can double as crypto/ROL seeds, so any edit here can corrupt downstream VM state — confirm polarity in debugger first |
| 2 (alternate, only if tracing shows 0xE2073A is the good path) | same | `75 16` → `74 16` (JNZ→JZ) | Invert the same check instead of forcing it | Same as #1, plus mutually exclusive with #1 |
| 3 (rejected — listed so nobody tries it) | 0xE238E2 / 0xA238E2 / 0x51B8E2 | `73 13` → `90 90` — **do not apply** | Entry-dispatcher flag branch; not a license gate (§4 DP-2) | **High**: feeds VM state; near-certain crash, zero launch benefit |
| — (explicit non-candidates) | slots 0x12028C5/0x1202A59/…; decoy 0x91B70F; .ini TITLEID | — (no edit) | Slots are runtime-overwritten with real API addresses; decoy is zero fill; TITLEID is data, not a code decision; activation servers are long dead (server-side, unpatchable client-side) | Editing any of these is either dead (overwritten/zeros) or off-target |

Smallest effective set: **edit #1 alone** (or #2 alone if polarity proves
inverted) — one 2-byte change at FO 0x518722. Everything else material to
launch (registry/disk/activation gates, the CreateProcess call itself) is
VM-wrapped with no static patch point **[OPEN]**.

## 7. Verification plan

1. **Back up**: `copy "F3Secu.exe" F3Secu.exe.orig` + record SHA-256; rollback
   = copy back (or Steam "Verify integrity"). Never edit the original in place
   without a restorable copy.
2. **Apply**: hex editor → FO **0x518722**, replace `75 16` with `90 90`.
   Re-checksum nothing (PE checksum is not enforced by the loader for this).
3. **Dynamic pre-check (recommended before editing)**: x64dbg/Olly, break at
   abs **0xE20722**, run; inspect ZF/`[EDI]` word. MZ-present images should
   fall through to 0xE20724 — confirms DP-1 polarity and that edit #1 (not #2)
   is the right one. Also break on `CreateProcessW` (slot 0x1202A59, FO
   0x6D0A59) to catch the launch call with its `STARTUPINFO`/command line.
4. **Run**: launch via `FableLauncher.exe` (production chain) or `F3Secu.exe`
   directly; monitor with Process Explorer/Process Monitor.
5. **Success looks like**: `Fable3.exe` child spawns (PPID = F3Secu), game
   window appears; `F3Secu.exe` stays resident until game exit
   (WaitForSingleObject behavior, §5). On this 2009-era title expect the
   *activation* leg to fail first (servers dead) — the MZ edit only clears the
   image check, so reaching a *different, later* failure (activation dialog /
   server-error string) is itself confirmation the edit worked.
6. **Roll back**: restore `F3Secu.exe.orig`; re-verify hash. If the game no
   longer starts at all after restore, Steam file verification repairs it.
7. **Open questions for dynamic work**: VM pass/fail gate location; exact
   CreateProcess call site + arguments; DFA handoff; disk-check ioctls;
   service-install trigger. All need a traced run, not more static reading —
   static recovery is exhausted (16/350 functions, zero xrefs into launch
   logic).
