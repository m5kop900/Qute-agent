#include "terminallauncher.h"

#include <QProcess>
#include <QStandardPaths>

namespace TerminalLauncher {

bool launch(const QString &program, const QStringList &args)
{
#if defined(Q_OS_WIN)
    // cmd /c start "" cmd /k <program> <args>
    QStringList a{QStringLiteral("/c"), QStringLiteral("start"), QStringLiteral(""), QStringLiteral("cmd"), QStringLiteral("/k"), program};
    a << args;
    return QProcess::startDetached(QStringLiteral("cmd.exe"), a);
#elif defined(Q_OS_MACOS)
    QString cmd = QStringLiteral("'%1' %2").arg(program, args.join(QLatin1Char(' ')));
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
        const QString exe = QStandardPaths::findExecutable(QString::fromLatin1(t.exe));
        if (exe.isEmpty())
            continue;
        QStringList a = t.prefix;
        a << program << args;
        if (QProcess::startDetached(exe, a))
            return true;
    }
    return false;
#endif
}

} // namespace TerminalLauncher
