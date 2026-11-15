#include "opencodelocator.h"

#include <QDir>
#include <QFileInfo>
#include <QProcess>
#include <QRegularExpression>
#include <QStandardPaths>

QStringList OpencodeLocator::commandNames()
{
    return {QStringLiteral("opencode"), QStringLiteral("opencode2")};
}

QStringList OpencodeLocator::extraSearchDirs()
{
    const QString h = QDir::homePath();
    return {h + QStringLiteral("/.opencode/bin"),  h + QStringLiteral("/.local/bin"),
            h + QStringLiteral("/bin"),            h + QStringLiteral("/.npm-global/bin"),
            h + QStringLiteral("/.bun/bin"),       QStringLiteral("/usr/local/bin"),
            QStringLiteral("/opt/homebrew/bin"),   QStringLiteral("/home/linuxbrew/.linuxbrew/bin")};
}

QStringList OpencodeLocator::searchPath()
{
    QStringList p = QString::fromLocal8Bit(qgetenv("PATH")).split(QDir::listSeparator(), Qt::SkipEmptyParts);
    for (const QString &e : extraSearchDirs())
        if (!p.contains(e))
            p << e;
    return p;
}

int OpencodeLocator::parseMajor(const QString &versionText)
{
    static const QRegularExpression re(QStringLiteral("(\\d+)\\.(\\d+)(?:\\.(\\d+))?"));
    const QRegularExpressionMatch m = re.match(versionText);
    return m.hasMatch() ? m.captured(1).toInt() : 0;
}

OpencodeBinary OpencodeLocator::probe(const QString &command, const QString &path)
{
    OpencodeBinary b;
    b.command = command;
    b.path = path;
    QProcess p;
    p.start(path, {QStringLiteral("--version")});
    if (!p.waitForStarted(3000))
        return b;
    if (!p.waitForFinished(8000)) {
        p.kill();
        p.waitForFinished(1000);
        return b;
    }
    b.runs = (p.exitStatus() == QProcess::NormalExit && p.exitCode() == 0);
    const QString out = QString::fromLocal8Bit(p.readAllStandardOutput()).trimmed();
    b.versionText = out.section(QLatin1Char('\n'), 0, 0).trimmed();
    b.major = parseMajor(b.versionText);
    return b;
}

QList<OpencodeBinary> OpencodeLocator::findAll()
{
    QList<OpencodeBinary> found;
    QStringList seen;
    const QStringList dirs = searchPath();
    for (const QString &name : commandNames()) {
        const QString path = QStandardPaths::findExecutable(name, dirs);
        if (path.isEmpty())
            continue;
        const QString canon = QFileInfo(path).canonicalFilePath();
        if (seen.contains(canon))
            continue;                       // opencode2 may be a symlink to opencode
        seen << canon;
        found << probe(name, path);
    }
    return found;
}

OpencodeBinary OpencodeLocator::best(const QString &channel)
{
    OpencodeBinary best;
    for (const OpencodeBinary &b : findAll()) {
        if (!b.runs)
            continue;
        if (channel == QLatin1String("v2") && b.major < 2)
            continue;
        if (channel == QLatin1String("v1") && b.major != 1)
            continue;
        if (best.command.isEmpty() || b.major > best.major)
            best = b;
    }
    return best;
}
