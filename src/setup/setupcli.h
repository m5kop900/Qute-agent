#pragma once
#include <QStringList>

// `qute opencode <install|setup|doctor|mcp|provider>` and `qute setup`.
// args = command line WITHOUT the program name, e.g. {"opencode", "install"} or {"setup", "--yes"}.
// Needs a QCoreApplication (no display, works over SSH).
class SetupCli {
public:
    static int run(const QStringList &args);
};
