#pragma once
#include <QList>
#include <QString>
#include <QStringList>

// Data-driven list of ways to install opencode. The upstream docs for V1/V2 changed several times
// (binary name `opencode` vs `opencode2`, package names, installers), so this table is the ONE place to update.
// Docs: https://opencode.ai/docs/ (V1 install), https://opencode.ai/v2/docs/ (V2 beta install),
//       https://opencode.ai/docs/migrate-v1.md
struct InstallOption {
    QString id;
    QString label;
    QString channel;        // "v2" or "v1"
    QString os;             // "linux", "macos", "windows", or "any"
    QString requiresTool;   // executable that must exist ("npm", "brew", "paru", "pacman", "curl", ...)
    QStringList argv;       // direct command (preferred)
    QString shell;          // alternative: a shell pipeline run through `sh -c` (curl | bash)
    bool needsSudo = false; // argv gets "sudo" prepended on Unix
    QString note;
    QString expectedCommand;// binary expected afterwards: "opencode" or "opencode2" (informational)

    QString displayCommand() const;
};

namespace OpencodeInstaller {
QString currentOs();
QList<InstallOption> allOptions();
QList<InstallOption> availableOptions(const QString &channel);   // for this OS and with the required tool present
// Runs the command with the terminal attached (so sudo/curl prompts work). Returns the process exit code.
int run(const InstallOption &option);
}
