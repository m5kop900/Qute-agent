#include "tty.h"

#include <QRegularExpression>
#include <QTextStream>
#include <iostream>
#include <string>

#if defined(Q_OS_UNIX)
#include <termios.h>
#include <unistd.h>
#elif defined(Q_OS_WIN)
#include <io.h>
#include <windows.h>
#endif

namespace Tty {

bool assumeYes = false;

bool interactive()
{
#if defined(Q_OS_UNIX)
    return isatty(STDIN_FILENO) && isatty(STDOUT_FILENO);
#elif defined(Q_OS_WIN)
    return _isatty(_fileno(stdin)) && _isatty(_fileno(stdout));
#else
    return true;
#endif
}

static bool useColor()
{
    static const bool c = interactive() && !qEnvironmentVariableIsSet("NO_COLOR");
    return c;
}

static QString paint(const char *code, const QString &t)
{
    return useColor() ? QStringLiteral("\x1b[%1m%2\x1b[0m").arg(QLatin1String(code), t) : t;
}

static void out(const QString &s)
{
    QTextStream o(stdout);
    o << s << "\n";
    o.flush();
}

static QString readLine(bool *eof = nullptr)
{
    std::string s;
    if (!std::getline(std::cin, s)) {
        if (eof) *eof = true;
        return {};
    }
    return QString::fromLocal8Bit(s.c_str()).trimmed();
}

static void prompt(const QString &s)
{
    QTextStream o(stdout);
    o << s;
    o.flush();
}

void heading(const QString &t) { out(QString()); out(paint("1;36", t)); out(paint("36", QString(t.size(), QLatin1Char('=')))); }
void step(int n, int total, const QString &t) { out(QString()); out(paint("1;35", QStringLiteral("[%1/%2] %3").arg(n).arg(total).arg(t))); }
void info(const QString &t) { out(QStringLiteral("  ") + t); }
void ok(const QString &t) { out(paint("32", QStringLiteral("  ✔ ")) + t); }
void warn(const QString &t) { out(paint("33", QStringLiteral("  ! ")) + t); }
void error(const QString &t) { out(paint("31", QStringLiteral("  ✘ ")) + t); }
void plain(const QString &t) { out(t); }

QString ask(const QString &question, const QString &def)
{
    if (!interactive() || assumeYes)
        return def;
    prompt(QStringLiteral("  %1%2: ").arg(question, def.isEmpty() ? QString() : QStringLiteral(" [%1]").arg(def)));
    const QString a = readLine();
    return a.isEmpty() ? def : a;
}

QString askSecret(const QString &question)
{
    if (!interactive())
        return {};
    prompt(QStringLiteral("  %1 (input hidden, empty = skip): ").arg(question));
#if defined(Q_OS_UNIX)
    termios oldt{};
    tcgetattr(STDIN_FILENO, &oldt);
    termios newt = oldt;
    newt.c_lflag &= ~static_cast<tcflag_t>(ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);
    const QString a = readLine();
    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
#elif defined(Q_OS_WIN)
    HANDLE h = GetStdHandle(STD_INPUT_HANDLE);
    DWORD mode = 0;
    GetConsoleMode(h, &mode);
    SetConsoleMode(h, mode & ~ENABLE_ECHO_INPUT);
    const QString a = readLine();
    SetConsoleMode(h, mode);
#else
    const QString a = readLine();
#endif
    prompt(QStringLiteral("\n"));
    return a;
}

bool confirm(const QString &question, bool def)
{
    if (assumeYes) {
        out(QStringLiteral("  %1 -> yes (--yes)").arg(question));
        return true;
    }
    if (!interactive())
        return def;
    for (;;) {
        prompt(QStringLiteral("  %1 [%2]: ").arg(question, def ? QStringLiteral("Y/n") : QStringLiteral("y/N")));
        bool eof = false;
        const QString a = readLine(&eof).toLower();
        if (eof || a.isEmpty()) return def;
        if (a == QLatin1String("y") || a == QLatin1String("yes")) return true;
        if (a == QLatin1String("n") || a == QLatin1String("no")) return false;
    }
}

int choose(const QString &title, const QStringList &options, int def)
{
    out(QStringLiteral("  %1").arg(paint("1", title)));
    for (int i = 0; i < options.size(); ++i)
        out(QStringLiteral("   %1) %2%3").arg(i + 1).arg(options[i], i == def ? paint("2", QStringLiteral("  (default)")) : QString()));
    if (!interactive() || assumeYes)
        return def;
    for (;;) {
        prompt(QStringLiteral("  Choose 1-%1 [%2]: ").arg(options.size()).arg(def + 1));
        bool eof = false;
        const QString a = readLine(&eof);
        if (eof || a.isEmpty()) return def;
        bool okNum = false;
        const int n = a.toInt(&okNum);
        if (okNum && n >= 1 && n <= options.size()) return n - 1;
    }
}

QList<int> chooseMany(const QString &title, const QStringList &options, const QList<int> &defaults)
{
    out(QStringLiteral("  %1").arg(paint("1", title)));
    for (int i = 0; i < options.size(); ++i)
        out(QStringLiteral("   %1) [%2] %3").arg(i + 1).arg(defaults.contains(i) ? QStringLiteral("x") : QStringLiteral(" "), options[i]));
    if (!interactive() || assumeYes)
        return defaults;
    prompt(QStringLiteral("  Numbers separated by comma, 'all', 'none', or Enter for the marked ones: "));
    const QString a = readLine().toLower();
    if (a.isEmpty()) return defaults;
    QList<int> res;
    if (a == QLatin1String("all")) { for (int i = 0; i < options.size(); ++i) res << i; return res; }
    if (a == QLatin1String("none")) return res;
    for (const QString &part : a.split(QRegularExpression(QStringLiteral("[,\\s]+")), Qt::SkipEmptyParts)) {
        bool okNum = false;
        const int n = part.toInt(&okNum);
        if (okNum && n >= 1 && n <= options.size() && !res.contains(n - 1)) res << n - 1;
    }
    return res;
}

} // namespace Tty
