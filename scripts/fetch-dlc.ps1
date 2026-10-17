#Requires -Version 5.1
# SPDX-License-Identifier: Apache-2.0
<#
.SYNOPSIS
  Fetch + install Fable 3 DLC packs for use with the unified xlive.dll.

.DESCRIPTION
  Same job as scripts/fetch-dlc.bat (and the .sh for Deck/Linux):
  1. copies assets/90_DLC_xlive_removal/ -> <game>/DLC/90_DLC_xlive_removal/
  2. downloads the 3 Xbox CDN .cab packs (resumable) into a cache dir
  3. extracts each cab root into <game>/DLC/0{1,2,3}_*/
  4. verifies content.xbx landed in each pack.

  Idempotent (skips finished packs) and resumable; -Force redo.
  Uses only inbox Windows 10+ tools (curl.exe, expand.exe, certutil via
  Get-FileHash). System32 tools are referenced by absolute path so the
  script also works from MSYS2/git-bash shells, where PATH may shadow
  `expand` with the unrelated coreutils text tool.

.EXAMPLE
  powershell -ExecutionPolicy Bypass -File scripts/fetch-dlc.ps1
.EXAMPLE
  powershell -ExecutionPolicy Bypass -File scripts/fetch-dlc.ps1 -GameDir 'D:\Games\Fable 3' -Force
#>
[CmdletBinding()]
param(
  [string]$GameDir = "",
  [string]$DlcCacheDir = "",
  [string]$BasePackDir = "",
  [switch]$Force
)

$ErrorActionPreference = 'Stop'
$sys32 = Join-Path $env:SystemRoot 'System32'
$curl = Join-Path $sys32 'curl.exe'
$expandExe = Join-Path $sys32 'expand.exe'
foreach ($tool in @($curl, $expandExe)) {
  if (-not (Test-Path $tool)) { throw "missing $tool (needs Windows 10+)" }
}

if (-not $GameDir) { $GameDir = $env:GAME_DIR }
if (-not $GameDir) {
  $GameDir = Join-Path ${env:ProgramFiles(x86)} 'Steam\steamapps\common\Fable 3'
}
if (-not (Test-Path (Join-Path $GameDir 'fable3.exe'))) {
  $alt = 'C:\Steam\steamapps\common\Fable 3\fable3.exe'
  if (Test-Path $alt) { $GameDir = Split-Path $alt }
}
if (-not (Test-Path (Join-Path $GameDir 'fable3.exe'))) {
  throw "game not found under '$GameDir' (pass -GameDir)"
}

$repoRoot = Split-Path $PSScriptRoot -Parent
if (-not $BasePackDir) { $BasePackDir = Join-Path $repoRoot 'assets\90_DLC_xlive_removal' }
if (-not $DlcCacheDir) { $DlcCacheDir = Join-Path $repoRoot '.dlc-cache' }
$dlcDir = Join-Path $GameDir 'DLC'
New-Item -ItemType Directory -Force $dlcDir, $DlcCacheDir | Out-Null

# dir | cab id | expected bytes | sha256
$packs = @(
  @('01_Understone',      '4d53090a0ecf0000', 256943166,  '869a0147251b109a8c14ad842e875b381c93c8e6a883a064d9c29b904697caae'),
  @('02_TraitorsKeep',    '4d53090a0ecf000e', 1025720613, '3728c92b4e4ef7f6c78fcf5720a98fcacb960d835d4e5c6d7ac5ff1d4fed9f79'),
  @('03_InquisitorsPack', '4d53090a0ecf0010', 1546685,     '836b94464b01f74e7cad6aeb7ec426b83aac12ecb180da6dfa9508730e136865')
)
$cabBase = 'http://download.xbox.com/content/4d53090a'

function Get-Sha256Hex([string]$Path) {
  # certutil-free hashing: Get-FileHash is missing on some PS 5.1 hosts.
  $sha = [System.Security.Cryptography.SHA256]::Create()
  try {
    $fs = [System.IO.File]::OpenRead($Path)
    try { $bytes = $sha.ComputeHash($fs) } finally { $fs.Close() }
    return ([BitConverter]::ToString($bytes)).Replace('-', '').ToLowerInvariant()
  } finally { $sha.Dispose() }
}

# 1. base stub pack
$stub = Join-Path $dlcDir '90_DLC_xlive_removal'
if ((Test-Path $stub) -and -not $Force) {
  Write-Host 'keep: DLC\90_DLC_xlive_removal already present'
} elseif (Test-Path (Join-Path $BasePackDir 'content.xbx')) {
  if (Test-Path $stub) { Remove-Item -Recurse -Force $stub }
  Copy-Item $BasePackDir $stub -Recurse -Force
  Write-Host 'copy: 90_DLC_xlive_removal\ <- assets\'
} else {
  Write-Warning "base pack missing at '$BasePackDir'; DLC enumeration needs DLC\90_DLC_xlive_removal\ (see docs\DLC.md)"
}

# 2-3. download + extract each pack
foreach ($p in $packs) {
  $dir, $cab, $wantSize, $wantSha = $p
  $dest = Join-Path $dlcDir $dir
  $cabFile = Join-Path $DlcCacheDir "$cab.cab"
  if ((Test-Path (Join-Path $dest 'content.xbx')) -and -not $Force) {
    Write-Host "keep: DLC\$dir already extracted"
    continue
  }
  Write-Host "== DLC\$dir"
  $size = if (Test-Path $cabFile) { (Get-Item $cabFile).Length } else { -1 }
  if ($size -ne $wantSize) {
    Write-Host "  get:  $cabBase/$cab.cab"
    & $curl -fSL --retry 3 -C - -o $cabFile "$cabBase/$cab.cab"
    if ($LASTEXITCODE -ne 0) { throw "curl failed for $cab.cab (exit $LASTEXITCODE)" }
  } else {
    Write-Host '  have: cached cab, size OK'
  }
  $size = (Get-Item $cabFile).Length
  if ($size -ne $wantSize) { throw "size mismatch for $cab.cab (got $size, want $wantSize)" }
  $gotSha = Get-Sha256Hex $cabFile
  if ($gotSha -ne $wantSha.ToLowerInvariant()) { throw "sha256 mismatch for $cab.cab" }
  Write-Host '  ok:   sha256 verified'
  New-Item -ItemType Directory -Force $dest | Out-Null
  Write-Host "  unpack -> DLC\$dir"
  & $expandExe $cabFile -F:* $dest | Out-Null
  if ($LASTEXITCODE -ne 0) { throw "expand failed for $cab.cab (exit $LASTEXITCODE)" }
  if (-not (Test-Path (Join-Path $dest 'content.xbx'))) {
    throw 'content.xbx missing after unpack'
  }
}

Write-Host ''
Write-Host "done. DLC layout under '$dlcDir':"
Get-ChildItem $dlcDir -Directory | ForEach-Object { Write-Host ("  " + $_.Name) }
Write-Host ''
Write-Host "next: set 'profile xuid 1' in xlive.ini to your save-folder id, then run."
Write-Host '(unified xlive.dll + xlive.ini go next to fable3.exe - see docs\BUILD.md)'
