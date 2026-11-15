#pragma once
#include "opencodeconfig.h"

#include <QList>
#include <QString>

// Curated MCP servers offered by the wizard. Each preset says what it needs so the wizard can check it.
struct McpPreset {
    QString id;                 // also the server name written to the config
    QString title;
    QString description;
    bool remote = false;
    QStringList command;        // local servers
    QString url;                // remote servers
    QString requiresTool;       // "npx" for node based servers, empty if none
    QString secretHeader;       // remote: header that carries an API key / token (empty = none)
    QString secretHeaderPrefix; // e.g. "Bearer "
    QString secretEnvName;      // env var suggested for the secret, e.g. CONTEXT7_API_KEY
    bool secretRequired = false;
    bool recommended = false;   // pre-selected in the wizard
    QString postInstallNote;
};

namespace McpPresets {
QList<McpPreset> all();
const McpPreset *find(const QString &id);
}
