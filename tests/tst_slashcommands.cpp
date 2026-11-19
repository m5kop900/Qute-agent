#include "slashcommands.h"

#include <QtTest>

static QVariantMap find(const QVariantList &items, const QString &key)
{
    for (const QVariant &v : items)
        if (v.toMap().value(QStringLiteral("key")).toString() == key)
            return v.toMap();
    return {};
}

static QStringList keys(const QVariantList &items)
{
    QStringList out;
    for (const QVariant &v : items)
        out << v.toMap().value(QStringLiteral("key")).toString();
    return out;
}

class TstSlashCommands : public QObject {
    Q_OBJECT
private slots:
    void compactIsBuiltinAndNative()
    {
        const QVariantList items = slashBuiltinItems();
        const QVariantMap compact = find(items, QStringLiteral("compact"));
        QVERIFY(!compact.isEmpty());
        QCOMPARE(compact.value("kind").toString(), QStringLiteral("native"));
        QCOMPARE(compact.value("label").toString(), QStringLiteral("/compact"));
        QVERIFY(compact.value("sub").toString().contains(QLatin1String("Summarize")));
        QCOMPARE(compact.value("aliases").toStringList(), QStringList{QStringLiteral("summarize")});
        QCOMPARE(compact.value("tag").toString(), QString());   // performed here: no TUI tag
    }

    void everyOpencodeBuiltinIsListed()
    {
        // The names of opencode 2's built-in commands (from the CLI build the project targets).
        // The commands Qute performs itself. Everything else is TUI-only and must not be listed.
        const QStringList expected = {
            "agents", "compact", "copy", "exit", "help", "models", "new", "sessions", "settings", "status",
        };
        const QStringList have = keys(slashBuiltinItems());
        for (const QString &name : expected)
            QVERIFY2(have.contains(name), qPrintable("missing /" + name));
    }

    void tuiOnlyCommandsAreHidden()
    {
        // Commands the overlay cannot perform are not in the "/" list at all (no dead entries, no TUI tag).
        const QStringList have = keys(slashBuiltinItems());
        for (const QString &name : {QStringLiteral("share"), QStringLiteral("connect"), QStringLiteral("diff"),
                                    QStringLiteral("themes"), QStringLiteral("undo")})
            QVERIFY2(!have.contains(name), qPrintable("TUI-only /" + name + " must be hidden"));
        for (const QVariant &v : slashBuiltinItems())
            QVERIFY(v.toMap().value("kind").toString() != QLatin1String("tui"));
    }

    void quteCommandsAreTagged()
    {
        const QVariantMap oi = find(slashBuiltinItems(), QStringLiteral("openinopencode"));
        QCOMPARE(oi.value("kind").toString(), QStringLiteral("qute"));
        QCOMPARE(oi.value("tag").toString(), QStringLiteral("QUTE"));
    }

    void serverCommandsAreAddedOnceAndBuiltinsWin()
    {
        QVariantList server;
        server << QVariantMap{{"name", "review"}, {"description", "Review the changes"}};
        server << QVariantMap{{"name", "compact"}, {"description", "server copy of compact"}};
        const QVariantList merged = mergeSlashItems(server);

        const QVariantMap review = find(merged, QStringLiteral("review"));
        QCOMPARE(review.value("kind").toString(), QStringLiteral("server"));
        QCOMPARE(review.value("sub").toString(), QStringLiteral("Review the changes"));

        // "compact" appears exactly once, and the built-in version wins.
        int compactCount = 0;
        for (const QString &k : keys(merged))
            compactCount += (k == QLatin1String("compact"));
        QCOMPARE(compactCount, 1);
        QCOMPARE(find(merged, QStringLiteral("compact")).value("kind").toString(), QStringLiteral("native"));
    }

    void emptyServerListStillShowsBuiltins()
    {
        // Regression: /api/command without a project location returns [] and the list used to be empty.
        QVERIFY(keys(mergeSlashItems({})).contains(QLatin1String("compact")));
    }

    void filterPutsPrefixMatchesFirst()
    {
        const QStringList hits = keys(filterSlashItems(slashBuiltinItems(), QStringLiteral("co")));
        QVERIFY(!hits.isEmpty());
        QVERIFY(hits.first().startsWith(QLatin1String("co")));
        QVERIFY(hits.contains(QLatin1String("compact")));
        QVERIFY(hits.contains(QLatin1String("copy")));
        // A name that only CONTAINS the text comes after the prefix matches.
        bool sawContainOnly = false;
        for (const QString &k : hits) {
            if (!k.startsWith(QLatin1String("co"))) sawContainOnly = true;
            else QVERIFY2(!sawContainOnly, "prefix match after a contains-only match");
        }
    }

    void filterMatchesAliasAndIgnoresCase()
    {
        QCOMPARE(keys(filterSlashItems(slashBuiltinItems(), QStringLiteral("SUMM"))), QStringList{QStringLiteral("compact")});
        QVERIFY(keys(filterSlashItems(slashBuiltinItems(), QStringLiteral("NEW"))).contains(QLatin1String("new")));
    }

    void emptyQueryKeepsOrder()
    {
        const QVariantList all = slashBuiltinItems();
        QCOMPARE(keys(filterSlashItems(all, QString())), keys(all));
    }

    void aliasesResolveToCanonicalNames()
    {
        QCOMPARE(canonicalSlashName(QStringLiteral("summarize")), QStringLiteral("compact"));
        QCOMPARE(canonicalSlashName(QStringLiteral("session")), QStringLiteral("sessions"));
        QCOMPARE(canonicalSlashName(QStringLiteral("compact")), QStringLiteral("compact"));
    }
};

QTEST_GUILESS_MAIN(TstSlashCommands)
#include "tst_slashcommands.moc"
