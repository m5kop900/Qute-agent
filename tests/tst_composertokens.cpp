#include "composertokens.h"

#include <QtTest>

class TstComposerTokens : public QObject {
    Q_OBJECT
private slots:
    void mentionAtStart()
    {
        const MentionToken t = findMentionToken(QStringLiteral("@src"), 4);
        QVERIFY(t.active);
        QCOMPARE(t.start, 0);
        QCOMPARE(t.query, QStringLiteral("src"));
    }

    void mentionAfterSpace()
    {
        const MentionToken t = findMentionToken(QStringLiteral("look at @fo"), 11);
        QVERIFY(t.active);
        QCOMPARE(t.start, 8);
        QCOMPARE(t.query, QStringLiteral("fo"));
    }

    void bareAtIsActiveWithEmptyQuery()
    {
        const MentionToken t = findMentionToken(QStringLiteral("@"), 1);
        QVERIFY(t.active);
        QCOMPARE(t.query, QString());
    }

    void emailIsNotAMention()
    {
        QVERIFY(!findMentionToken(QStringLiteral("mail me at a@b"), 14).active);
    }

    void cursorInsideTokenUsesTextBeforeCursor()
    {
        const MentionToken t = findMentionToken(QStringLiteral("@foo bar"), 2);
        QVERIFY(t.active);
        QCOMPARE(t.query, QStringLiteral("f"));
        QCOMPARE(t.end, 2);
    }

    void spaceAfterMentionEndsIt()
    {
        QVERIFY(!findMentionToken(QStringLiteral("@a "), 3).active);
    }

    void noAtNoMention()
    {
        QVERIFY(!findMentionToken(QStringLiteral("hello world"), 5).active);
        QVERIFY(!findMentionToken(QString(), 0).active);
    }
};

QTEST_GUILESS_MAIN(TstComposerTokens)
#include "tst_composertokens.moc"
