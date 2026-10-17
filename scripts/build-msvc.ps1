#Requires -Version 5.1
<#
.SYNOPSIS
  Build 32-bit xlive.dll with MSVC (x86).

.DESCRIPTION
  Sets up the MSVC x86 environment for this session (PATH/INCLUDE/LIB,
  no persistent changes) and runs the same cl command as CI
  (.github/workflows/build.yml). Run from anywhere; output lands in
  the repo root (xlive.dll) and is staged into dist\ with xlive.ini.

  If 'cl' is not on PATH this script finds it via vswhere (or the
  default Community install path) -- no manual PATH editing needed.

.EXAMPLE
  # One-time per shell if scripts are blocked:
  Set-ExecutionPolicy -Scope Process Bypass
  .\scripts\build-msvc.ps1

.EXAMPLE
  .\scripts\build-msvc.ps1 -Verify   # also run scripts\check_exports.py (needs python+pefile)
#>
[CmdletBinding()]
param(
  [switch]$Verify,
  [switch]$NoClean,
  [string]$VsPath,
  [string]$SdkVersion
)

$ErrorActionPreference = 'Stop'
$root = Split-Path $PSScriptRoot -Parent

# --- Locate Visual Studio ----------------------------------------------------
if (-not $VsPath) {
  $vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
  if (Test-Path $vswhere) {
    $VsPath = & $vswhere -latest -products '*' `
      -requires Microsoft.VisualStudio.Component.VC.Tools `
      -property installationPath 2>$null
  }
  if (-not $VsPath) {
    $VsPath = 'C:\Program Files\Microsoft Visual Studio\2022\Community'
  }
}
if (-not (Test-Path $VsPath)) { throw "VS not found at: $VsPath" }

$msvcBase = Get-ChildItem "$VsPath\VC\Tools\MSVC" -Directory -Filter '14.*' |
  Sort-Object Name -Descending | Select-Object -First 1
if (-not $msvcBase) { throw "No MSVC toolchain under $VsPath\VC\Tools\MSVC" }

$msvcBin = Join-Path $msvcBase.FullName 'bin\Hostx64\x86'
if (-not (Test-Path (Join-Path $msvcBin 'cl.exe'))) {
  $msvcBin = Join-Path $msvcBase.FullName 'bin\Hostx86\x86'  # 32-bit host fallback
}
if (-not (Test-Path (Join-Path $msvcBin 'cl.exe'))) { throw "cl.exe not found under $($msvcBase.FullName)" }

# --- Locate Windows SDK -------------------------------------------------------
$sdkRoot = 'C:\Program Files (x86)\Windows Kits\10'
if (-not $SdkVersion) {
  $SdkVersion = Get-ChildItem "$sdkRoot\Lib" -Directory -Filter '10.0.*' |
    Sort-Object Name -Descending | Select-Object -First 1 -ExpandProperty Name
}
foreach ($d in @("Include\$SdkVersion\um", "Lib\$SdkVersion\um\x86")) {
  if (-not (Test-Path (Join-Path $sdkRoot $d))) { throw "Windows SDK $SdkVersion incomplete (missing $d)" }
}

# --- Session environment (no persistent changes) ------------------------------
$env:PATH = "$msvcBin;$env:PATH"
$env:INCLUDE = "$($msvcBase.FullName)\include;$sdkRoot\Include\$SdkVersion\ucrt;$sdkRoot\Include\$SdkVersion\um;$sdkRoot\Include\$SdkVersion\shared"
$env:LIB = "$($msvcBase.FullName)\lib\x86;$sdkRoot\Lib\$SdkVersion\ucrt\x86;$sdkRoot\Lib\$SdkVersion\um\x86"
Write-Host "MSVC $($msvcBase.Name) + SDK $SdkVersion" -ForegroundColor Cyan

# --- Build (same flags as CI) -------------------------------------------------
Push-Location $root
try {
  cl /LD /O2 /W3 /WX /std:clatest /D_CRT_SECURE_NO_WARNINGS `
    /Fe:xlive.dll src\*.c src\xlive.def `
    ws2_32.lib shell32.lib user32.lib
  if ($LASTEXITCODE -ne 0) { throw "cl failed (exit $LASTEXITCODE)" }

  New-Item -ItemType Directory -Force dist | Out-Null
  Copy-Item xlive.dll dist\ -Force
  Copy-Item src\xlive.ini dist\ -Force
  certutil -hashfile dist\xlive.dll SHA256

  $dumpbin = Join-Path $msvcBin 'dumpbin.exe'
  $n = (& $dumpbin /exports xlive.dll | Select-String '^\s+\d+\s+[0-9A-F]+\s+\S').Count
  Write-Host "exports: $n (expect 249)" -ForegroundColor Cyan

  if ($Verify) {
    foreach ($py in @('py', 'python')) {
      if (Get-Command $py -ErrorAction SilentlyContinue) {
        & $py scripts\check_exports.py xlive.dll
        if ($LASTEXITCODE -ne 0) { throw 'export check failed' }
        break
      }
    }
  }

  if (-not $NoClean) { Remove-Item *.obj, *.lib, *.exp -ErrorAction SilentlyContinue }
  Write-Host 'OK: xlive.dll + dist\' -ForegroundColor Green
}
finally { Pop-Location }
