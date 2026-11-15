#pragma once
#include <QList>
#include <QString>
#include <QStringList>

// Minimal terminal UI helpers (numbered menus, prompts, colors). No curses dependency, works in any terminal.
// In non-interactive mode (stdin is not a TTY, or --yes) every question returns its default.
namespace Tty {
extern bool assumeYes;
bool interactive();

void heading(const QString &text);
void step(int n, int total, const QString &text);
void info(const QString &text);
void ok(const QString &text);
void warn(const QString &text);
void error(const QString &text);
void plain(const QString &text);

QString ask(const QString &question, const QString &def = QString());
QString askSecret(const QString &question);
bool confirm(const QString &question, bool def = true);
int choose(const QString &title, const QStringList &options, int def = 0);                       // returns index
QList<int> chooseMany(const QString &title, const QStringList &options, const QList<int> &defaults);
}
