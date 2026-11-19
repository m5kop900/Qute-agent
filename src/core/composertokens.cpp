#include "composertokens.h"

#include "slashcommands.h"

#include <QChar>

MentionToken findMentionToken(const QString &text, int cursor)
{
    MentionToken none;
    if (cursor < 0 || cursor > text.size())
        return none;
    int i = cursor - 1;
    while (i >= 0 && !text.at(i).isSpace())
        --i;
    const int at = i + 1;                       // first character of the token
    if (at >= cursor || text.at(at) != QLatin1Char('@'))
        return none;
    if (at > 0 && !text.at(at - 1).isSpace())    // "a@b" is not a mention
        return none;
    MentionToken t;
    t.active = true;
    t.start = at;
    t.end = cursor;
    t.query = text.mid(at + 1, cursor - at - 1);
    return t;
}

QVariantMap ComposerTools::mentionAt(const QString &text, int cursor) const
{
    const MentionToken t = findMentionToken(text, cursor);
    return QVariantMap{{QStringLiteral("active"), t.active}, {QStringLiteral("start"), t.start},
                       {QStringLiteral("end"), t.end}, {QStringLiteral("query"), t.query}};
}

QVariantList ComposerTools::filterSlash(const QVariantList &items, const QString &query) const
{
    return filterSlashItems(items, query);
}

QString ComposerTools::canonicalCommand(const QString &name) const
{
    return canonicalSlashName(name);
}
