#include "slashcommands.h"

#include <QVariantMap>

namespace {

struct Builtin {
    const char *name;
    const char *title;
    const char *alias;   // "" when none
    bool native;         // true: Qute performs it through the API; false: TUI only
};

// The opencode built-in commands that Qute performs itself, through the API. Every other built-in is TUI-only and is
// deliberately NOT listed: a command the overlay cannot run would only be noise in the "/" list. Titles follow the
// opencode 2 CLI. "native" entries are the only kind in this table.
const Builtin kOpencodeBuiltins[] = {
    {"agents", "Switch agent (build, plan, ...)", "", true},
    {"compact", "Summarize the session to reduce context size", "summarize", true},
    {"copy", "Copy the last answer to the clipboard", "", true},
    {"exit", "Hide Qute", "", true},
    {"help", "Show how the commands work", "", true},
    {"models", "Switch model", "", true},
    {"new", "Start a new session", "", true},
    {"sessions", "Switch session", "session", true},
    {"settings", "Open settings", "", true},
    {"status", "Show agent, model, session and context", "", true},
};

QVariantMap item(const QString &key, const QString &sub, const QString &tag, const QString &kind,
                 const QStringList &aliases = {})
{
    return QVariantMap{{QStringLiteral("key"), key},
                       {QStringLiteral("label"), QStringLiteral("/") + key},
                       {QStringLiteral("sub"), sub},
                       {QStringLiteral("tag"), tag},
                       {QStringLiteral("kind"), kind},
                       {QStringLiteral("aliases"), aliases}};
}

} // namespace

QString canonicalSlashName(const QString &name)
{
    if (name == QLatin1String("session")) return QStringLiteral("sessions");
    if (name == QLatin1String("summarize")) return QStringLiteral("compact");
    if (name == QLatin1String("directory")) return QStringLiteral("directory");
    return name;
}

QVariantList slashBuiltinItems()
{
    QVariantList out;
    // Qute's own commands first (they have no opencode counterpart).
    out << item(QStringLiteral("openinopencode"), QObject::tr("Open this session in the opencode TUI"),
                QStringLiteral("QUTE"), QStringLiteral("qute"));
    out << item(QStringLiteral("directory"), QObject::tr("Set the project folder (starts a new session)"),
                QStringLiteral("QUTE"), QStringLiteral("qute"));
    for (const Builtin &b : kOpencodeBuiltins) {
        const QStringList aliases = QString::fromLatin1(b.alias).isEmpty()
                                        ? QStringList{}
                                        : QStringList{QString::fromLatin1(b.alias)};
        out << item(QString::fromLatin1(b.name), QObject::tr(b.title),
                    b.native ? QString() : QStringLiteral("TUI"),
                    b.native ? QStringLiteral("native") : QStringLiteral("tui"), aliases);
    }
    return out;
}

QVariantList mergeSlashItems(const QVariantList &serverCommands)
{
    QVariantList out = slashBuiltinItems();
    QStringList taken;
    for (const QVariant &v : out) {
        const QVariantMap m = v.toMap();
        taken << m.value(QStringLiteral("key")).toString();
        for (const QVariant &a : m.value(QStringLiteral("aliases")).toStringList())
            taken << a.toString();
    }
    for (const QVariant &v : serverCommands) {
        const QVariantMap m = v.toMap();
        const QString name = m.value(QStringLiteral("name")).toString();
        if (name.isEmpty() || taken.contains(name))
            continue;
        taken << name;
        out << item(name, m.value(QStringLiteral("description")).toString(), QString(),
                    QStringLiteral("server"));
    }
    return out;
}

QVariantList filterSlashItems(const QVariantList &items, const QString &query)
{
    const QString q = query.trimmed().toLower();
    if (q.isEmpty())
        return items;
    QVariantList prefix;
    QVariantList contains;
    for (const QVariant &v : items) {
        const QVariantMap m = v.toMap();
        const QString key = m.value(QStringLiteral("key")).toString().toLower();
        bool startsWith = key.startsWith(q);
        bool hasContaining = key.contains(q);
        for (const QVariant &a : m.value(QStringLiteral("aliases")).toStringList()) {
            if (a.toString().toLower().startsWith(q)) startsWith = true;
            if (a.toString().toLower().contains(q)) hasContaining = true;
        }
        if (startsWith)
            prefix << v;
        else if (hasContaining)
            contains << v;
    }
    return prefix + contains;
}
