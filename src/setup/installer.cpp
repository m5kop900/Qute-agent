#include "installer.h"

#include <QProcess>
#include <QStandardPaths>

QString InstallOption::displayCommand() const
{
    if (!shell.isEmpty())
        return shell;
    QStringList parts = argv;
    if (needsSudo)
        parts.prepend(QStringLiteral("sudo"));
    return parts.join(QLatin1Char(' '));
}

namespace OpencodeInstaller {

QString currentOs()
{
#if defined(Q_OS_WIN)
    return QStringLiteral("windows");
#elif defined(Q_OS_MACOS)
    return QStringLiteral("macos");
#else
    return QStringLiteral("linux");
#endif
}

static InstallOption opt(const QString &id, const QString &label, const QString &channel, const QString &os,
                         const QString &tool, const QStringList &argv, const QString &shell = {},
                         bool sudo = false, const QString &note = {}, const QString &expected = QStringLiteral("opencode"))
{
    InstallOption o;
    o.id = id; o.label = label; o.channel = channel; o.os = os; o.requiresTool = tool;
    o.argv = argv; o.shell = shell; o.needsSudo = sudo; o.note = note; o.expectedCommand = expected;
    return o;
}

QList<InstallOption> allOptions()
{
    QList<InstallOption> o;
    // ---- OpenCode 2 ----
    o << opt("v2-pacman", "Arch Linux official package (pacman)", "v2", "linux", "pacman",
             {"pacman", "-S", "opencode"}, {}, true, "The official Arch package installs OpenCode 2.");
    o << opt("v2-brew", "Homebrew (official formula)", "v2", "any", "brew",
             {"brew", "install", "opencode"}, {}, false, "The official Homebrew formula currently installs OpenCode 2.");
    o << opt("v2-npm", "npm (beta channel)", "v2", "any", "npm",
             {"npm", "install", "-g", "@opencode/cli@beta"}, {}, false,
             "Beta docs: installs the `opencode2` command next to V1.", "opencode2");
    o << opt("v2-aur-paru", "AUR beta package (paru)", "v2", "linux", "paru",
             {"paru", "-S", "opencode-beta"}, {}, false, "Provides the `opencode2` command.", "opencode2");
    o << opt("v2-aur-yay", "AUR beta package (yay)", "v2", "linux", "yay",
             {"yay", "-S", "opencode-beta"}, {}, false, "Provides the `opencode2` command.", "opencode2");
    o << opt("v2-curl", "Official V2 install script (curl | bash)", "v2", "linux", "curl", {},
             "curl -fsSL https://opencode.ai/v2/install | bash", false,
             "Downloads and runs a script from opencode.ai. Read it first if you care: https://opencode.ai/v2/install");
    o << opt("v2-curl-mac", "Official V2 install script (curl | bash)", "v2", "macos", "curl", {},
             "curl -fsSL https://opencode.ai/v2/install | bash");
    // ---- OpenCode 1 ----
    o << opt("v1-curl", "Official V1 install script (curl | bash)", "v1", "linux", "curl", {},
             "curl -fsSL https://opencode.ai/install | bash", false,
             "Downloads and runs a script from opencode.ai. It installs the `opencode` binary (may replace V2).");
    o << opt("v1-curl-mac", "Official V1 install script (curl | bash)", "v1", "macos", "curl", {},
             "curl -fsSL https://opencode.ai/install | bash");
    o << opt("v1-npm", "npm (opencode-ai)", "v1", "any", "npm", {"npm", "install", "-g", "opencode-ai"});
    o << opt("v1-brew", "Homebrew tap", "v1", "any", "brew", {"brew", "install", "anomalyco/tap/opencode"});
    o << opt("v1-aur-paru", "AUR (paru, opencode-bin)", "v1", "linux", "paru", {"paru", "-S", "opencode-bin"});
    o << opt("v1-aur-yay", "AUR (yay, opencode-bin)", "v1", "linux", "yay", {"yay", "-S", "opencode-bin"});
    o << opt("v1-choco", "Chocolatey", "v1", "windows", "choco", {"choco", "install", "opencode"});
    o << opt("v1-scoop", "Scoop", "v1", "windows", "scoop", {"scoop", "install", "opencode"});
    return o;
}

QList<InstallOption> availableOptions(const QString &channel)
{
    QList<InstallOption> res;
    const QString os = currentOs();
    for (const InstallOption &o : allOptions()) {
        if (o.channel != channel)
            continue;
        if (o.os != QLatin1String("any") && o.os != os)
            continue;
        if (!o.requiresTool.isEmpty() && QStandardPaths::findExecutable(o.requiresTool).isEmpty())
            continue;
        if (o.needsSudo && os != QLatin1String("windows") && QStandardPaths::findExecutable(QStringLiteral("sudo")).isEmpty())
            continue;
        res << o;
    }
    return res;
}

int run(const InstallOption &option)
{
    QProcess p;
    p.setProcessChannelMode(QProcess::ForwardedChannels);
    p.setInputChannelMode(QProcess::ForwardedInputChannel);     // lets sudo / installers ask questions
    if (!option.shell.isEmpty()) {
#if defined(Q_OS_WIN)
        return 127;                                             // no shell pipelines on Windows
#else
        p.start(QStringLiteral("/bin/sh"), {QStringLiteral("-c"), option.shell});
#endif
    } else {
        QStringList argv = option.argv;
        if (option.needsSudo && currentOs() != QLatin1String("windows"))
            argv.prepend(QStringLiteral("sudo"));
        const QString program = argv.takeFirst();
        p.start(program, argv);
    }
    if (!p.waitForStarted(10000))
        return 127;
    p.waitForFinished(-1);
    return p.exitStatus() == QProcess::NormalExit ? p.exitCode() : 128;
}

} // namespace OpencodeInstaller
