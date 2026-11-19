#Requires -Version 5.1
<#
.SYNOPSIS
    Creates Start Menu shortcuts for Qute Agent and optionally starts the daemon at login.

.DESCRIPTION
    Qute Agent has no built-in global shortcut: the desktop launches `qute toggle`. This script creates
    per-user shortcuts (no admin rights needed):
      * "Qute Agent"          -> qute.exe toggle     (optionally with a global hotkey on the .lnk)
      * "Qute Agent Settings" -> qute-settings.exe
      * "Qute Agent daemon"   -> qute.exe --daemon   (only with -Autostart, in the Startup folder)

.PARAMETER BinDir
    Folder containing qute.exe. Default: look for qute.exe on PATH.

.PARAMETER Hotkey
    Optional global hotkey for the overlay shortcut, e.g. "CTRL+ALT+Q" (modifiers: CTRL, ALT, SHIFT, WIN).

.PARAMETER Autostart
    Also put a daemon shortcut into the current user's Startup folder so the first toggle is instant.

.EXAMPLE
    powershell -ExecutionPolicy Bypass -File packaging\windows\install-shortcuts.ps1 `
        -BinDir .\dist\qute-agent\bin -Hotkey "CTRL+ALT+Q" -Autostart
#>
[CmdletBinding()]
param(
    [string]$BinDir = "",
    [string]$Hotkey = "",
    [switch]$Autostart
)

$ErrorActionPreference = "Stop"

function Find-BinDir {
    param([string]$Dir)
    if ($Dir) {
        $full = if ([System.IO.Path]::IsPathRooted($Dir)) { $Dir } else { Join-Path (Get-Location) $Dir }
        if (-not (Test-Path -LiteralPath (Join-Path $full "qute.exe"))) {
            throw "qute.exe not found in $full"
        }
        return (Resolve-Path -LiteralPath $full).Path
    }
    $cmd = Get-Command "qute.exe" -ErrorAction SilentlyContinue
    if (-not $cmd) { throw "qute.exe not found on PATH; pass -BinDir <folder with qute.exe>" }
    return (Split-Path -Parent $cmd.Source)
}

$bin = Find-BinDir $BinDir
$qute = Join-Path $bin "qute.exe"
$settings = Join-Path $bin "qute-settings.exe"
$shell = New-Object -ComObject WScript.Shell

function New-Shortcut {
    param(
        [string]$Path,
        [string]$Target,
        [string]$Arguments = "",
        [string]$Description = "",
        [string]$ShortcutHotkey = ""
    )
    $lnk = $shell.CreateShortcut($Path)
    $lnk.TargetPath = $Target
    $lnk.Arguments = $Arguments
    $lnk.WorkingDirectory = $bin
    $lnk.IconLocation = "$Target,0"
    $lnk.Description = $Description
    if ($ShortcutHotkey) { $lnk.Hotkey = $ShortcutHotkey }
    $lnk.Save()
    Write-Host "created: $Path"
}

$programs = Join-Path ([Environment]::GetFolderPath("Programs")) "Qute Agent"
New-Item -ItemType Directory -Force -Path $programs | Out-Null

$overlayLnk = Join-Path $programs "Qute Agent.lnk"
try {
    New-Shortcut -Path $overlayLnk -Target $qute -Arguments "toggle" `
        -Description "Show or hide the Qute Agent overlay" -ShortcutHotkey $Hotkey
} catch {
    # An invalid Hotkey string makes Save() fail; retry without it instead of leaving no shortcut at all.
    if (-not $Hotkey) { throw }
    Write-Warning "hotkey '$Hotkey' was rejected by Windows; creating the shortcut without a hotkey"
    New-Shortcut -Path $overlayLnk -Target $qute -Arguments "toggle" `
        -Description "Show or hide the Qute Agent overlay"
}

if (Test-Path -LiteralPath $settings) {
    New-Shortcut -Path (Join-Path $programs "Qute Agent Settings.lnk") -Target $settings `
        -Description "Configure the Qute Agent overlay"
} else {
    Write-Warning "qute-settings.exe not found next to qute.exe; skipping the settings shortcut"
}

if ($Autostart) {
    $startup = [Environment]::GetFolderPath("Startup")
    New-Shortcut -Path (Join-Path $startup "Qute Agent daemon.lnk") -Target $qute -Arguments "--daemon" `
        -Description "Start the Qute Agent overlay daemon at login (hidden)"
}

Write-Host ""
Write-Host "Done. Start Menu folder: $programs"
if ($Hotkey) { Write-Host "Hotkey: $Hotkey (global while the shortcut exists)" }
if ($Autostart) { Write-Host "Autostart: $([Environment]::GetFolderPath('Startup'))" }
Write-Host "Remove everything again by deleting the shortcuts listed above."
