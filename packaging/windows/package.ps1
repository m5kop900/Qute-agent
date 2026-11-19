#Requires -Version 5.1
<#
.SYNOPSIS
    Builds a portable Qute Agent zip for Windows (executables + Qt runtime + sprite packs).

.DESCRIPTION
    Copies qute.exe and qute-settings.exe into bin\, runs windeployqt for both, copies the sprite packs to
    share\qute\spritepacks (the layout SpritePackManager searches), and writes
    dist\qute-agent-<version>-windows-<arch>.zip.

.PARAMETER BuildDir
    CMake build directory. Default: build

.PARAMETER OutDir
    Where the stage folder and the zip are written. Default: dist

.PARAMETER Config
    Configuration for multi-config generators (Visual Studio): Release or Debug. Empty for single-config.

.PARAMETER QtBin
    Folder that contains windeployqt.exe. Default: found on PATH.

.EXAMPLE
    powershell -ExecutionPolicy Bypass -File packaging\windows\package.ps1 -BuildDir build -OutDir dist

.EXAMPLE
    powershell -ExecutionPolicy Bypass -File packaging\windows\package.ps1 -BuildDir build -Config Release `
        -QtBin C:\Qt\6.8.0\msvc2022_64\bin
#>
[CmdletBinding()]
param(
    [string]$BuildDir = "build",
    [string]$OutDir = "dist",
    [string]$Config = "",
    [string]$QtBin = ""
)

$ErrorActionPreference = "Stop"

$Repo = (Resolve-Path (Join-Path $PSScriptRoot "..\..")).Path
function Get-FullPath([string]$path) {
    if ([System.IO.Path]::IsPathRooted($path)) { return $path }
    return (Join-Path (Get-Location) $path)
}
$BuildDir = (Resolve-Path (Get-FullPath $BuildDir)).Path
$OutDir = Get-FullPath $OutDir

function Resolve-Exe {
    param([string]$SubDir, [string]$Exe)
    $dir = Join-Path $BuildDir $SubDir
    $candidates = @()
    if ($Config) { $candidates += Join-Path $dir "$Config\$Exe" }
    $candidates += Join-Path $dir $Exe
    if (Test-Path $dir) {
        $candidates += @(Get-ChildItem -Path $dir -Filter $Exe -Recurse -File -ErrorAction SilentlyContinue |
            ForEach-Object { $_.FullName })
    }
    foreach ($c in $candidates) {
        if (Test-Path -LiteralPath $c -PathType Leaf) { return (Resolve-Path -LiteralPath $c).Path }
    }
    throw "$Exe not found under $dir. Build first: cmake --build $BuildDir"
}

$qute = Resolve-Exe "src\overlay" "qute.exe"
$settings = Resolve-Exe "src\settings" "qute-settings.exe"

# Version from CMakeLists.txt (project(qute VERSION x.y.z ...)) unless overridden.
$version = "0.0.0"
$cmakeText = Get-Content -Raw (Join-Path $Repo "CMakeLists.txt")
if ($cmakeText -match 'project\s*\(\s*qute\s+VERSION\s+([0-9]+\.[0-9]+\.[0-9]+)') {
    $version = $Matches[1]
}

$arch = if ($env:PROCESSOR_ARCHITECTURE -eq "ARM64") { "arm64" } else { "x64" }

$stage = Join-Path $OutDir "qute-agent"
if (Test-Path $stage) { Remove-Item -Recurse -Force $stage }
$binDir = Join-Path $stage "bin"
$shareDir = Join-Path $stage "share\qute"
New-Item -ItemType Directory -Force -Path $binDir, $shareDir | Out-Null

Copy-Item -LiteralPath $qute -Destination $binDir
Copy-Item -LiteralPath $settings -Destination $binDir
Copy-Item -Path (Join-Path $Repo "assets\spritepacks") -Destination $shareDir -Recurse
Get-ChildItem -Path (Join-Path $shareDir "spritepacks") -Filter "preview.png" -Recurse -File |
    Remove-Item -Force
Copy-Item -LiteralPath (Join-Path $Repo "README.md") -Destination $stage
Copy-Item -LiteralPath (Join-Path $Repo "packaging\windows\README.md") -Destination $stage
foreach ($file in @("install-shortcuts.ps1", "qute.ahk")) {
    Copy-Item -LiteralPath (Join-Path $PSScriptRoot $file) -Destination $stage
}

$windeployqt = $null
if ($QtBin) {
    $windeployqt = Join-Path $QtBin "windeployqt.exe"
} else {
    $cmd = Get-Command "windeployqt.exe" -ErrorAction SilentlyContinue
    if ($cmd) { $windeployqt = $cmd.Source }
}
if (-not $windeployqt -or -not (Test-Path -LiteralPath $windeployqt)) {
    throw "windeployqt.exe not found. Add the Qt bin folder to PATH or pass -QtBin <Qt>\bin"
}

# --qmldir lets windeployqt find the Qt QML modules the app imports; the app's own QML is compiled in.
$deployArgs = @(
    "--release",
    "--compiler-runtime",
    "--no-translations",
    "--qmldir", (Join-Path $Repo "src\overlay")
)
foreach ($exe in @($qute, $settings)) {
    Write-Host "windeployqt: $exe"
    & $windeployqt @deployArgs $exe
    if ($LASTEXITCODE -ne 0) { throw "windeployqt failed for $exe (exit $LASTEXITCODE)" }
}

$zip = Join-Path $OutDir "qute-agent-$version-windows-$arch.zip"
if (Test-Path $zip) { Remove-Item -Force $zip }
Compress-Archive -Path $stage -DestinationPath $zip -CompressionLevel Optimal
$hash = (Get-FileHash -Algorithm SHA256 -LiteralPath $zip).Hash

$sizeMb = [math]::Round((Get-Item $zip).Length / 1MB, 1)
Write-Host ""
Write-Host "Packaged: $zip ($sizeMb MB)"
Write-Host "SHA256  : $hash"
Write-Host "Run     : $stage\bin\qute.exe toggle"
