#pragma once
#include <QList>
#include <QString>
#include <QStringList>

struct OpencodeBinary {
    QString command;      // "opencode" or "opencode2"
    QString path;         // absolute path
    QString versionText;  // first line printed by `--version`
    int major = 0;        // 0 = could not parse
    bool runs = false;    // `--version` exited normally
};

// Finds installed opencode binaries (V1 = `opencode`; some V2 betas install `opencode2`).
// Looks in PATH plus the places the official installers use but that are often missing from PATH.
class OpencodeLocator {
public:
    static QStringList commandNames();       // {"opencode", "opencode2"}
    static QStringList extraSearchDirs();    // ~/.opencode/bin, ~/.local/bin, ...
    static QStringList searchPath();         // PATH + extraSearchDirs
    static QList<OpencodeBinary> findAll();
    // channel: "v2", "v1" or "" (highest major version wins). Empty command => nothing suitable found.
    static OpencodeBinary best(const QString &channel = QString());
    static OpencodeBinary probe(const QString &command, const QString &path);
    static int parseMajor(const QString &versionText);
};
