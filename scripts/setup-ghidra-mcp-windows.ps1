# SPDX-License-Identifier: Apache-2.0
<#
.SYNOPSIS
  Native-Windows Ghidra + ReVa MCP setup (companion to docs/GHIDRA.md).

.DESCRIPTION
  Idempotent: safe to re-run. Installs Temurin JDK 21 + uv via winget,
  downloads Ghidra 12.1.2 + ReVa extension, seeds the extension into the
  Ghidra install (UI + headless), installs the headless mcp-reva bridge
  (with the required mcp<2 pin), persists user env vars, and registers
  the deferred 'ReVa-headless' server with pi.

  The Nix flake (flake.nix) covers Linux/macOS/WSL only — Nix has no
  native Windows port, hence this script.

.EXAMPLE
  .\scripts\setup-ghidra-mcp-windows.ps1
.EXAMPLE
  .\scripts\setup-ghidra-mcp-windows.ps1 -InstallDir 'D:\tools' -SkipWinget
#>
[CmdletBinding()]
param(
  [string]$InstallDir = 'C:\Tools',
  [string]$GhidraVersion = '12.1.2',
  [string]$GhidraDate = '20260605',
  [string]$RevaVersion = '7.3.1',
  [string]$RevaDate = '20260903',
  [switch]$SkipWinget
)

$ErrorActionPreference = 'Stop'

$GhidraZipName = "ghidra_${GhidraVersion}_PUBLIC_${GhidraDate}.zip"
$GhidraUrl = "https://github.com/NationalSecurityAgency/ghidra/releases/download/Ghidra_${GhidraVersion}_build/${GhidraZipName}"
$GhidraDir = Join-Path $InstallDir "ghidra_${GhidraVersion}_PUBLIC"
# ReVa ships one zip per supported Ghidra minor (12.1 zip covers 12.1.x).
$RevaGhidraMinor = ($GhidraVersion -split '\.')[0..1] -join '.'
$RevaZipName = "ghidra_${RevaGhidraMinor}_PUBLIC_reva.zip"
$RevaUrl = "https://github.com/cyberkaida/reverse-engineering-assistant/releases/download/v${RevaVersion}/ghidra_${RevaGhidraMinor}_PUBLIC_${RevaDate}_reverse-engineering-assistant.zip"
$DlDir = Join-Path $InstallDir 'ghidra-mcp-downloads'

function Invoke-Step([string]$Name, [scriptblock]$Body) {
  Write-Host "`n== $Name" -ForegroundColor Cyan
  & $Body
}

function Get-FileWithRetry([string]$Url, [string]$Dest) {
  if (Test-Path $Dest) { Write-Host "have: $Dest"; return }
  Write-Host "get:  $Url"
  curl.exe -fSL --retry 3 -C - -o $Dest $Url
  if ($LASTEXITCODE -ne 0) { throw "download failed: $Url" }
}

function Expand-ZipNet([string]$Zip, [string]$DestDir) {
  Add-Type -AssemblyName System.IO.Compression.FileSystem
  # .NET ExtractToDirectory throws if DestDir exists; extract to temp then merge.
  $tmp = Join-Path ([IO.Path]::GetTempPath()) ("ghidra-unzip-" + [IO.Path]::GetRandomFileName())
  try {
    [System.IO.Compression.ZipFile]::ExtractToDirectory($Zip, $tmp)
    Get-ChildItem $tmp | ForEach-Object {
      $target = Join-Path $DestDir $_.Name
      if (Test-Path $target) { Remove-Item $target -Recurse -Force }
      Move-Item $_.FullName $target
    }
  } finally { if (Test-Path $tmp) { Remove-Item $tmp -Recurse -Force } }
}

Invoke-Step 'prereqs (JDK 21 + uv)' {
  if (-not $SkipWinget) {
    winget install --id EclipseAdoptium.Temurin.21.JDK -e --silent `
      --accept-source-agreements --accept-package-agreements
    winget install --id astral-sh.uv -e --silent `
      --accept-source-agreements --accept-package-agreements
  }
  # Re-resolve PATH for this process (winget mutates user/machine PATH).
  $env:Path = [Environment]::GetEnvironmentVariable('Path', 'Machine') + ';' +
              [Environment]::GetEnvironmentVariable('Path', 'User')
  $uv = (Get-Command uv.exe -ErrorAction Stop).Source
  Write-Host "uv: $uv"
  $javaCmd = Get-Command java.exe -ErrorAction SilentlyContinue
  if ($null -eq $javaCmd) { throw 'java.exe not found — install Temurin JDK 21 first' }
  try { $javaOut = (& java.exe -version 2>&1 | Out-String) } catch { $javaOut = '' }
  Write-Host (($javaOut -split '\r?\n' | Select-Object -First 1))
}

Invoke-Step 'download Ghidra + ReVa' {
  New-Item -ItemType Directory -Force -Path $DlDir | Out-Null
  Get-FileWithRetry $GhidraUrl (Join-Path $DlDir $GhidraZipName)
  Get-FileWithRetry $RevaUrl (Join-Path $DlDir $RevaZipName)
}

Invoke-Step 'extract Ghidra' {
  if (-not (Test-Path (Join-Path $GhidraDir 'ghidraRun.bat'))) {
    Expand-ZipNet (Join-Path $DlDir $GhidraZipName) $InstallDir
  }
  Write-Host "ghidra: $GhidraDir"
}

Invoke-Step 'seed ReVa extension into install' {
  # Must live in Ghidra\Extensions (headless/pyghidra only sees the install
  # dir; the UI also reads it, so one copy serves both — no user-dir dup).
  $extRoot = Join-Path $GhidraDir 'Ghidra\Extensions\reverse-engineering-assistant'
  if (-not (Test-Path (Join-Path $extRoot 'extension.properties'))) {
    $tmp = Join-Path ([IO.Path]::GetTempPath()) ('reva-' + [IO.Path]::GetRandomFileName())
    try {
      Expand-ZipNet (Join-Path $DlDir $RevaZipName) $tmp
      $src = Join-Path $tmp 'reverse-engineering-assistant'
      if (-not (Test-Path (Join-Path $src 'extension.properties'))) { throw 'unexpected ReVa zip layout' }
      if (Test-Path $extRoot) { Remove-Item $extRoot -Recurse -Force }
      Move-Item $src $extRoot
    } finally { if (Test-Path $tmp) { Remove-Item $tmp -Recurse -Force } }
  }
  Get-Content (Join-Path $extRoot 'extension.properties') | Select-Object -First 4
}

Invoke-Step 'headless bridge (mcp-reva)' {
  # ReVa <= 7.3.1 needs the MCP 1.x SDK (2.x renamed streamablehttp_client).
  $uvExe = (Get-Command uv.exe).Source
  $listed = & $uvExe tool list 2>$null | Out-String
  if ($listed -match 'reverse-engineering-assistant') {
    Write-Host "have: $($listed.Trim())"
  } else {
    & $uvExe tool install "reverse-engineering-assistant==${RevaVersion}" `
      --with 'mcp>=1.28.1,<2'
  }
}

Invoke-Step 'user env vars' {
  $javaHome = Get-ChildItem 'C:\Program Files\Eclipse Adoptium' -Directory `
    | Where-Object { $_.Name -like 'jdk-21*' } | Sort-Object Name -Descending `
    | Select-Object -First 1 -ExpandProperty FullName
  if (-not $javaHome) { throw 'Temurin JDK 21 not found under C:\Program Files\Eclipse Adoptium' }
  [Environment]::SetEnvironmentVariable('GHIDRA_INSTALL_DIR', $GhidraDir, 'User')
  [Environment]::SetEnvironmentVariable('JAVA_HOME', $javaHome, 'User')
  $up = [Environment]::GetEnvironmentVariable('Path', 'User')
  $localBin = Join-Path $env:USERPROFILE '.local\bin'
  if ($up -notlike "*$localBin*") {
    [Environment]::SetEnvironmentVariable('Path', "$localBin;$up", 'User')
  }
  $env:GHIDRA_INSTALL_DIR = $GhidraDir
  $env:JAVA_HOME = $javaHome
  Write-Host "GHIDRA_INSTALL_DIR=$GhidraDir"
  Write-Host "JAVA_HOME=$javaHome"
}

Invoke-Step 'register pi MCP server' {
  $mcpReva = Join-Path $env:USERPROFILE '.local\bin\mcp-reva.exe'
  if (-not (Test-Path $mcpReva)) { throw "missing $mcpReva" }
  pi mcp add ReVa-headless --exposure deferred `
    --env "GHIDRA_INSTALL_DIR=$GhidraDir" `
    --env "JAVA_HOME=$env:JAVA_HOME" `
    --description 'ReVa Ghidra reverse-engineering MCP (headless). Use for xlive.dll analysis.' `
    -- $mcpReva
}

Write-Host "`n== verify" -ForegroundColor Cyan
pi mcp list
Write-Host @'

Next (assistant/UI mode, optional): run ghidraRun.bat, open a project,
enable 'ReVa Application Plugin' (project File > Configure) and 'ReVa
Plugin' (Code Browser File > Configure, then Save Tool). Then:
  pi mcp add ReVa --url http://127.0.0.1:8080/mcp/message
See docs/GHIDRA.md for details.
'@
