#pragma once
#include <QStringList>

// Opens a terminal emulator running the given command (used by the overlay's "Set up opencode" button,
// because the setup wizard is an interactive terminal program). Platform specific -> lives in src/platform.
namespace TerminalLauncher {
bool launch(const QString &program, const QStringList &args);
}
