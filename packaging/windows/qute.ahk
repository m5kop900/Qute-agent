; Qute Agent global hotkey for Windows — AutoHotkey v2 (https://www.autohotkey.com/).
;
; Qute Agent has no built-in global shortcut by design: a desktop keybinding runs `qute toggle`.
; On Windows the simplest option is this script; the alternative is install-shortcuts.ps1 -Hotkey.
;
; Usage:
;   1. Install AutoHotkey v2.
;   2. Put qute.exe in a bin\ folder next to this script, or add it to PATH.
;   3. Double-click this file (or create a shortcut to it in shell:startup for autostart).
;
; Ctrl+Alt+Q  show/hide the overlay (starts the daemon automatically on first use)

#Requires AutoHotkey v2.0
#SingleInstance Force

quteExe := A_ScriptDir . "\bin\qute.exe"
if !FileExist(quteExe)
    quteExe := "qute.exe"

^!q::Run('"' . quteExe . '" toggle')

; Ctrl+Alt+, opens the settings app (optional; remove if the hotkey is taken).
^!s::Run('"' . quteExe . '" settings')
