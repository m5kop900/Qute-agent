#pragma once
#include <QString>
#include <QVariantList>

// The "/" command list shown above the prompt box (F1.4).
//
// Three sources are merged, in this order:
//  1. Qute's own commands (kind "qute"): run by Qute only.
//  2. opencode's built-in commands that Qute performs itself (kind "native"; see slashcommands.cpp). Every other
//     opencode built-in is TUI-only and is left out of the list on purpose.
//  3. Commands the server knows (kind "server"): the project's own commands from GET /api/command.
//
// Each item is a map: key (name without "/"), label ("/name"), sub (description), tag ("QUTE" or ""),
// kind, aliases (list of alternative names that also match and run the item).
// Pure data code (no Qt GUI, no network) so it can be unit tested.

// Maps an alias (e.g. "summarize", "session", "toggle-thinking") to its canonical command name.
// Unknown names are returned unchanged.
QString canonicalSlashName(const QString &name);

// Built-in items: Qute's own commands followed by opencode's built-in table.
QVariantList slashBuiltinItems();

// Built-ins plus the server's commands (server items whose name collides with a built-in are dropped).
QVariantList mergeSlashItems(const QVariantList &serverCommands);

// Filters items by the text typed after "/": names/aliases that start with the text come first, then names
// that contain it. Case-insensitive. An empty query returns every item in the original order.
QVariantList filterSlashItems(const QVariantList &items, const QString &query);
