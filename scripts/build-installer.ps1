<#
.SYNOPSIS
    Builds the KINGFN Browser release and packages it with NSIS.
.DESCRIPTION
    1. Compiles kingfn_browser in Release mode via CMake.
    2. Runs makensis to produce the setup EXE.
.EXAMPLE
    .\scripts\build-installer.ps1
    .\scripts\build-installer.ps1 -SkipBuild   # NSIS only
#>
param(
    [switch]$SkipBuild,
    [string]$BuildDir = "out\cef",
    [string]$NsisExe  = "C:\Program Files (x86)\NSIS\makensis.exe"
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

$Root = Split-Path $PSScriptRoot -Parent
Push-Location $Root

Write-Host "`n=== KINGFN Browser Build & Package ===" -ForegroundColor Cyan

# ── Step 1: CMake build ────────────────────────────────────────────────────────
if (-not $SkipBuild) {
    Write-Host "`n[1/2] Configuring CMake..." -ForegroundColor Yellow
    cmake -S . -B $BuildDir `
        -G "Visual Studio 17 2022" -A x64 `
        -DKINGFN_ENABLE_CEF=ON `
        -DUSE_SANDBOX=OFF
    if ($LASTEXITCODE -ne 0) { throw "CMake configure failed." }

    Write-Host "`n[1/2] Building kingfn_browser (Release)..." -ForegroundColor Yellow
    cmake --build $BuildDir --config Release --target kingfn_browser -j
    if ($LASTEXITCODE -ne 0) { throw "CMake build failed." }
    Write-Host "[1/2] Build complete." -ForegroundColor Green
} else {
    Write-Host "[1/2] Skipping build (--SkipBuild)." -ForegroundColor DarkGray
}

# ── Step 2: NSIS installer ────────────────────────────────────────────────────
Write-Host "`n[2/2] Creating installer with NSIS..." -ForegroundColor Yellow

if (-not (Test-Path $NsisExe)) {
    Write-Warning "NSIS not found at '$NsisExe'."
    Write-Warning "Download from https://nsis.sourceforge.io and rerun."
    Write-Warning "Or pass -NsisExe 'C:\path\to\makensis.exe'."
    exit 1
}

& $NsisExe /V3 "installer\kingfn-setup.nsi"
if ($LASTEXITCODE -ne 0) { throw "NSIS packaging failed." }

$SetupFile = Get-ChildItem -Filter "kingfn-browser-setup-*.exe" | Select-Object -Last 1
if ($SetupFile) {
    $SizeMB = [math]::Round($SetupFile.Length / 1MB, 1)
    Write-Host "`n=== Installer ready ===" -ForegroundColor Green
    Write-Host "  File : $($SetupFile.FullName)"
    Write-Host "  Size : $SizeMB MB"
} else {
    Write-Warning "Setup EXE not found — check NSIS output above."
}

Pop-Location
