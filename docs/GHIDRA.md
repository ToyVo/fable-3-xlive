# Ghidra + ReVa MCP setup

Two supported environments for AI-assisted reverse engineering of the
reference `xlive.dll`s (see [PROVENANCE.md](PROVENANCE.md)). Both use
**Ghidra 12.1.x** with the **ReVa** extension
([cyberkaida/reverse-engineering-assistant](https://github.com/cyberkaida/reverse-engineering-assistant)),
which serves the MCP tools. Pick one:

| | Nix (Linux / macOS / WSL) | Native Windows (this file) |
|---|---|---|
| Ghidra | `nix run .#ghidra` → 12.1.2 + ReVa 7.3.0 baked in | `C:\Tools\ghidra_12.1.2_PUBLIC` + ReVa 7.3.1, manual install below |
| MCP client | Claude Code per ReVa README, or `pi` headless | `pi` headless (global, on demand) and/or assistant (Ghidra UI open) |
| Setup cost | one command | ~10 min, mostly downloads (~600 MB) |

> Why not Nix on Windows? `flake.nix` only defines `aarch64-darwin`,
> `aarch64-linux`, `x86_64-linux` — Nix has no native Windows port. On
> Windows you can still use the Nix env **inside WSL**
> (`wsl -d NixOS` / Ubuntu + Nix flakes → `nix run .#ghidra`), but Ghidra's
> UI then needs an X server / WSLg. The native setup below avoids that.

## Fast path (Windows)

Run the idempotent installer from a PowerShell prompt (JDK install may
prompt for admin/UAC):

```powershell
.\scripts\setup-ghidra-mcp-windows.ps1
```

It installs Temurin JDK 21 + `uv` via winget, downloads Ghidra 12.1.2 and
the ReVa extension, installs the headless `mcp-reva` bridge, sets user env
vars, and registers the `ReVa-headless` server with `pi`. Then verify:

```powershell
pi mcp list   # want: ReVa-headless: connected, 86 tools
```

The rest of this file explains what the script does, so you can do it by
hand or debug it.

## Manual setup (Windows)

Pinned versions (verified 2026-10-10; bump together — ReVa zips are
per-Ghidra-version builds):

- Ghidra **12.1.2** (`20260605`):
  `https://github.com/NationalSecurityAgency/ghidra/releases/download/Ghidra_12.1.2_build/ghidra_12.1.2_PUBLIC_20260605.zip`
- ReVa **7.3.1** for Ghidra 12.1:
  `https://github.com/cyberkaida/reverse-engineering-assistant/releases/download/v7.3.1/ghidra_12.1_PUBLIC_20260903_reverse-engineering-assistant.zip`
- (Nix currently resolves ReVa 7.3.0 for the same Ghidra 12.1.x — the two
  are interchangeable for analysis; Windows uses 7.3.1 because its bridge
  needs the `mcp<2` pin below.)

### 1. Prereqs: JDK 21 + uv

```powershell
winget install --id EclipseAdoptium.Temurin.21.JDK -e --silent --accept-source-agreements --accept-package-agreements
winget install --id astral-sh.uv -e --silent --accept-source-agreements --accept-package-agreements
```

Refresh `PATH` afterwards (open a new terminal). Check with
`java -version` (want 21.x) and `uv --version`.

### 2. Ghidra

Unzip the Ghidra release to `C:\Tools\` so you get
`C:\Tools\ghidra_12.1.2_PUBLIC\ghidraRun.bat`. Smoke-test:

```powershell
$env:JAVA_HOME = 'C:\Program Files\Eclipse Adoptium\jdk-21.0.12.101-hotspot'
C:\Tools\ghidra_12.1.2_PUBLIC\support\analyzeHeadless.bat  # prints usage → OK
```

### 3. ReVa extension — put it in the install dir

Unzip the ReVa `ghidra_12.1_PUBLIC_*` file and copy the
`reverse-engineering-assistant/` folder to
`C:\Tools\ghidra_12.1.2_PUBLIC\Ghidra\Extensions\`.

This location matters: the Ghidra UI also reads per-user
`%USERPROFILE%\.ghidra\...​\Extensions\`, but **headless/pyghidra only sees
extensions inside the install dir**. Installing in `Ghidra\Extensions\`
serves both, and mirrors what `ghidra.withExtensions` does on Nix. Do not
keep a second copy in the user dir — Ghidra flags duplicate installs.

### 4. Headless bridge (`mcp-reva`)

```powershell
uv tool install 'reverse-engineering-assistant==7.3.1' --with 'mcp>=1.28.1,<2'
```

The `--with 'mcp>=1.28.1,<2'` pin is required: ReVa ≤ 7.3.1 imports
`streamablehttp_client` from the MCP 1.x SDK, while unpinned `uv` resolves
MCP 2.x, which renamed it to `streamable_http_client`
(`ImportError` otherwise; upstream fixed this on `main` after 7.3.1).

Persist env for manual runs (the installer script does this with
`setx`-equivalent calls):

| var | value |
|---|---|
| `GHIDRA_INSTALL_DIR` | `C:\Tools\ghidra_12.1.2_PUBLIC` |
| `JAVA_HOME` | `C:\Program Files\Eclipse Adoptium\jdk-21.0.12.101-hotspot` |
| `PATH` | prepend `%USERPROFILE%\.local\bin` (where `mcp-reva.exe` lives) |

### 5. Enable the plugins (assistant/UI mode only)

Headless mode needs nothing more. For interactive work with the Ghidra UI:

1. Run `C:\Tools\ghidra_12.1.2_PUBLIC\ghidraRun.bat`, open/create a project.
2. Project view → File → Configure → plug icon → check **ReVa Application
   Plugin**.
3. Open a program (Code Browser) → File → Configure → plug icon → check
   **ReVa Plugin** → File → Save Tool.

ReVa then serves MCP over streamable HTTP on port 8080
(`http://localhost:8080/mcp/message`, configurable in Tool Options).

### 6. Register with `pi`

Headless (works any time, ephemeral per-session projects, no UI needed).
`deferred` keeps normal `pi` sessions fast — Ghidra (~60 s boot) only
starts when you actually invoke a ReVa tool:

```powershell
pi mcp add ReVa-headless --exposure deferred `
  --env 'GHIDRA_INSTALL_DIR=C:\Tools\ghidra_12.1.2_PUBLIC' `
  --env 'JAVA_HOME=C:\Program Files\Eclipse Adoptium\jdk-21.0.12.101-hotspot' `
  --description 'ReVa Ghidra reverse-engineering MCP (headless). Use for xlive.dll analysis.' `
  -- "$env:USERPROFILE\.local\bin\mcp-reva.exe"
```

Assistant (only when the Ghidra UI from step 5 is open with a project).
Opt-in — don't add it globally or `pi mcp list` goes red whenever Ghidra
is closed:

```powershell
pi mcp add ReVa --url http://127.0.0.1:8080/mcp/message `
  --description 'ReVa in-Ghidra assistant (needs Ghidra UI open).'
pi mcp remove ReVa   # when done with UI work
```

## Verify end to end

```powershell
pi mcp list
# ReVa-headless: connected, 86 tools (deferred, global)
```

Deeper check — raw MCP handshake over stdio (boots headless Ghidra, takes
1–3 min, prints `SERVER: ReVa …` + `TOOLS: 86`):

```powershell
$env:GHIDRA_INSTALL_DIR = 'C:\Tools\ghidra_12.1.2_PUBLIC'
$env:JAVA_HOME = 'C:\Program Files\Eclipse Adoptium\jdk-21.0.12.101-hotspot'
uv run --python 3.12 --with 'mcp>=1.28.1,<2' --with mcp-reva `
  python -c "import asyncio,os; ..."   # full script in setup log / ask
```

First real analysis: ask `pi` to `import-file` one of the reference DLLs
(hashes in [PROVENANCE.md](PROVENANCE.md)), then `analyze-program` +
`get-decompilation` on `FUN_10009350` / `FUN_100012d0` and compare with
the anchors in [COMPARISON.md](COMPARISON.md).

## Security notes

- ReVa exposes a PyGhidra scripting tool. Keep it on localhost (the
  default); if you bind it publicly, enable the API-key auth or disable the
  scripting tool group (`mcp-reva --disable-tool-group scripting`).
- The reference DLLs are untrusted third-party binaries: analyze them, don't
  execute them. Headless projects are ephemeral temp dirs, which limits
  residue.

## Troubleshooting

| symptom | fix |
|---|---|
| `ImportError: streamablehttp_client` | reinstall bridge with `--with 'mcp>=1.28.1,<2'` (§4) |
| `No module named 'reva'` from `mcp-reva` | extension must be in `Ghidra\Extensions\`, not only the user dir (§3) |
| `Java was not found in PATH or JAVA_HOME` | set `JAVA_HOME` (+ `java` on `PATH`) before launching; the `pi` server entry already carries both via `--env` |
| Ghidra UI doesn't list ReVa | extension zip must contain the top-level `reverse-engineering-assistant/` folder with `extension.properties`; re-copy per §3 |
| `pi mcp list` shows ReVa-headless failing | check the two `--env` paths still exist (JDK minor updates move `JAVA_HOME`); rerun the setup script |
| winget JDK install asks for admin | expected (installs to Program Files); accept the UAC prompt or pre-install Temurin 21 yourself and set `JAVA_HOME` |
