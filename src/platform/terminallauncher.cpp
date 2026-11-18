#include "terminallauncher.h"

#include <QProcess>
#include <QStandardPaths>

namespace TerminalLauncher {

namespace {
// POSIX single-quote escaping for a path that is pasted into a shell command (macOS only).
QString shellQuote(const QString &s)
{
    QString out = s;
    out.replace(QLatin1Char('\''), QStringLiteral("'\\''"));
    return QLatin1Char('\'') + out + QLatin1Char('\'');
}
} // namespace

bool launch(const QString &program, const QStringList &args, const QString &workDir)
{
#if defined(Q_OS_WIN)
    // cmd /c start "" cmd /k <program> <args>; the working directory is inherited from the launching process.
    QStringList a{QStringLiteral("/c"), QStringLiteral("start"), QStringLiteral(""), QStringLiteral("cmd"), QStringLiteral("/k"), program};
    a << args;
    return QProcess::startDetached(QStringLiteral("cmd.exe"), a, workDir);
#elif defined(Q_OS_MACOS)
    QString cmd;
    if (!workDir.isEmpty())
        cmd = QStringLiteral("cd %1 && ").arg(shellQuote(workDir));
    // Every argument is quoted, so a URL, password or session id with spaces or quotes stays one argument.
    QStringList quoted{shellQuote(program)};
    for (const QString &a : args)
        quoted << shellQuote(a);
    cmd += quoted.join(QLatin1Char(' '));
    cmd.replace(QLatin1Char('\\'), QStringLiteral("\\\\"));                 // AppleScript string escaping
    cmd.replace(QLatin1Char('"'), QStringLiteral("\\\""));
    return QProcess::startDetached(QStringLiteral("osascript"),
        {QStringLiteral("-e"), QStringLiteral("tell application \"Terminal\" to do script \"%1\"").arg(cmd), QStringLiteral("-e"),
         QStringLiteral("tell application \"Terminal\" to activate")});
#else
    struct Term { const char *exe; QStringList prefix; };
    const QList<Term> terms = {
        {"konsole", {"--hold", "-e"}},        {"gnome-terminal", {"--"}},      {"xfce4-terminal", {"--hold", "-x"}},
        {"qterminal", {"-e"}},                {"lxterminal", {"-e"}},          {"kitty", {"--hold"}},
        {"alacritty", {"--hold", "-e"}},      {"wezterm", {"start", "--"}},    {"foot", {"-H"}},
        {"x-terminal-emulator", {"-e"}},      {"xterm", {"-hold", "-e"}},
    };
    for (const Term &t : terms) {
        const QString name = QString::fromLatin1(t.exe);
        const QString exe = QStandardPaths::findExecutable(name);
        if (exe.isEmpty())
            continue;
        QStringList a;
        // Terminal-specific working-directory flags go before the program; the rest use startDetached's cwd.
        if (!workDir.isEmpty()) {
            if (name == QLatin1String("konsole"))
                a << QStringLiteral("--workdir") << workDir;
            else if (name == QLatin1String("gnome-terminal") || name == QLatin1String("xfce4-terminal"))
                a << (QStringLiteral("--working-directory=") + workDir);
        }
        a << t.prefix << program << args;
        if (QProcess::startDetached(exe, a, workDir))
            return true;
    }
    return false;
#endif
}

} // namespace TerminalLauncher
