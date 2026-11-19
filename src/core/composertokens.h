#pragma once
#include <QObject>
#include <QString>
#include <QVariantList>
#include <QVariantMap>

// The "@" token under the cursor in the prompt box (F1.3). A mention starts with "@" at the beginning of the
// text or right after whitespace, and runs up to the cursor with no whitespace in between. "a@b.com" is not a
// mention. Pure function, unit tested.
struct MentionToken {
    bool active = false;
    int start = -1;      // index of the "@"
    int end = -1;        // cursor position (exclusive end of the token)
    QString query;       // text after the "@" up to the cursor
};

MentionToken findMentionToken(const QString &text, int cursor);

// QML-facing helpers for the prompt box (registered as "composerTools"). Pure logic only.
class ComposerTools : public QObject {
    Q_OBJECT
public:
    using QObject::QObject;
    // {active, start, end, query} for the "@" token at the cursor.
    Q_INVOKABLE QVariantMap mentionAt(const QString &text, int cursor) const;
    // Filters "/" items by the text after the slash (prefix matches first).
    Q_INVOKABLE QVariantList filterSlash(const QVariantList &items, const QString &query) const;
    // Alias -> canonical command name ("summarize" -> "compact").
    Q_INVOKABLE QString canonicalCommand(const QString &name) const;
};
