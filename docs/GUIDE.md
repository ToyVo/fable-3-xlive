# Fable 3 on Steam/Deck: single-player + DLC setup

Get Fable 3 running without Games for Windows LIVE, with all 3 DLC packs, using
the unified `xlive.dll` from this repo (open source, [how it works](COMPARISON.md)).
No mystery Dropbox DLLs, no manual CAB juggling — scripts do the heavy lifting.

You need: a copy of Fable 3 (Steam), ~1.5 GB free if you want the DLC, and about
20 minutes (mostly downloads).

## Part 1 — the replacement DLL

1. Download the latest release from this repo's **Releases** page:
   `xlive.dll`, `xlive.ini` (and `fetch-dlc.bat` / `fetch-dlc.sh` — you'll need
   one of them in Part 2). Optional: check the DLL against `xlive.dll.sha256`.
2. Put `xlive.dll` + `xlive.ini` in the Fable 3 folder, replacing any old
   `xlive.dll` (the DLC will *not* work with the old Timeslip one):
   - **PC:** `C:\Program Files (x86)\Steam\steamapps\common\Fable 3`
   - **Deck:** switch to Desktop Mode, open Dolphin, go to
     `/home/deck/.local/share/Steam/steamapps/common/Fable 3/`
3. Run Fable 3 once. There are no GFWL sign-in screens anymore — every online
   prompt is answered locally and silently.
   - If it stalls on loading datasets/updates at 4/4, close it, switch to
     Gaming Mode (Deck) or restart it (PC), and run it again so the check can
     finish.

If you only want the base game, you're done — enjoy! For DLC, continue.

## Part 2 — the 3 DLC packs (scripted)

The script downloads Understone, Traitor's Keep, and the Inquisitor's Pack from
Microsoft's own Xbox archive (~1.24 GB total, resumable), verifies every file,
and lays out the `DLC\` folder exactly as the DLL expects. Re-running it is
safe: finished packs are skipped.

**PC:**

1. Put `fetch-dlc.bat` (from the release) anywhere — the Fable 3 folder is
   simplest — and double-click it. (Needs Windows 10+; `curl`/`expand` are
   built in.) To target a non-Steam install, run it with the folder as an
   argument instead.
2. Wait for the three downloads + `done.` summary.

**Deck:**

1. On the Releases page, also grab the **source ZIP** of this repo and extract
   it (the script needs the `assets/` folder next to it).
2. Open Konsole, `cd` into the extracted folder, and run:
   ```sh
   ./scripts/fetch-dlc.sh
   ```
   It defaults to the Deck Steam path. Wait for the `done.` summary.

You should end up with:
```
Fable 3/DLC/
  90_DLC_xlive_removal/
  01_Understone/
  02_TraitorsKeep/
  03_InquisitorsPack/
```

## Part 3 — bind your save (both platforms)

The DLL keys saves to the profile in `xlive.ini`, so point it at your save:

1. Find your save folder and copy its name (looks like `1000100010001000`):
   - **PC:** `C:\Users\<you>\Saved Games\Lionhead Studios\Fable 3`
   - **Deck:** inside the Proton prefix, usually
     `/home/deck/.local/share/Steam/steamapps/compatdata/105400/pfx/drive_c/users/steamuser/Saved Games/Lionhead Studios/Fable 3/`
2. Open `xlive.ini` (the one next to `fable3.exe`), find
   `profile xuid 1 = 1122334400000000`, replace the number with your save
   folder's name, save the file.

## Part 4 — check and troubleshoot

Run Fable 3 and head to the Sanctuary: DLC gifts waiting on the left, by where
the dog lays, mean everything enumerated correctly.

- **No DLC?** Confirm the DLL next to `fable3.exe` is the unified one from the
  release (verify with `xlive.dll.sha256`), the four `DLC\` folders exist,
  and each pack contains `content.xbx`.
- **Save won't load?** The XUID in step 2 of Part 3 must match the save folder
  exactly.
- **Something else?** Set `debug log = 1` in `xlive.ini`, reproduce, and read
  `xlive_trace.log` next to the exe — include it if you open an issue.

Enjoy! :)
