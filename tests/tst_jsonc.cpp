#include "jsonc.h"
#include <QJsonDocument>
#include <QtTest>

class TstJsonc : public QObject {
    Q_OBJECT
private slots:
    void stripsLineAndBlockComments()
    {
        bool had = false;
        const QByteArray out = Jsonc::strip("{\n // hi\n \"a\": 1, /* x */ \"b\": 2\n}", &had);
        QVERIFY(had);
        QJsonParseError e;
        const auto doc = QJsonDocument::fromJson(out, &e);
        QCOMPARE(e.error, QJsonParseError::NoError);
        QCOMPARE(doc.object().value("b").toInt(), 2);
    }
    void keepsCommentLookalikesInsideStrings()
    {
        // Regular strings, not raw literals: moc does not understand R"(...)" and
        // treats a later "//" as a comment, then drops the Q_OBJECT class.
        const QByteArray out = Jsonc::strip("{\"url\":\"https://x.y/a//b\",\"c\":\"/* no */\"}");
        const auto o = QJsonDocument::fromJson(out).object();
        QCOMPARE(o.value("url").toString(), QString("https://x.y/a//b"));
        QCOMPARE(o.value("c").toString(), QString("/* no */"));
    }
    void removesTrailingCommas()
    {
        const QByteArray out = Jsonc::strip("{\"a\":[1,2,],\"b\":{\"c\":1,},}");
        QJsonParseError e;
        QJsonDocument::fromJson(out, &e);
        QCOMPARE(e.error, QJsonParseError::NoError);
    }
    void handlesEscapedQuotes()
    {
        const QByteArray out = Jsonc::strip("{\"a\":\"he said \\\"//\\\" ok\"}");
        QCOMPARE(QJsonDocument::fromJson(out).object().value("a").toString(), QString("he said \"//\" ok"));
    }
};

QTEST_APPLESS_MAIN(TstJsonc)
#include "tst_jsonc.moc"
