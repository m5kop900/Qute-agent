#pragma once
#include <QStringList>

// Opens a terminal emulator running the given command (used by the overlay's "Set up opencode" button,
// because the setup wizard is an interactive terminal program). Platform specific -> lives in src/platform.
namespace TerminalLauncher {
// workDir (optional) becomes the terminal's working directory; the program is passed as an argv list, never a shell string.
bool launch(const QString &program, const QStringList &args, const QString &workDir = QString());
}
