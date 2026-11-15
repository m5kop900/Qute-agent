#include "sseparser.h"
#include <QtTest>

class TstSseParser : public QObject {
    Q_OBJECT
private slots:
    void singleEvent()
    {
        SseParser p;
        const auto ev = p.feed("event: server.connected\ndata: {}\n\n");
        QCOMPARE(ev.size(), 1);
        QCOMPARE(ev[0].event, QString("server.connected"));
        QCOMPARE(ev[0].data, QByteArray("{}"));
    }
    void splitAcrossChunks()
    {
        SseParser p;
        QVERIFY(p.feed("data: {\"type\":\"a\"").isEmpty());
        QVERIFY(p.feed("}\n").isEmpty());
        const auto ev = p.feed("\n");
        QCOMPARE(ev.size(), 1);
        QCOMPARE(ev[0].data, QByteArray("{\"type\":\"a\"}"));
    }
    void multiLineDataAndCrlf()
    {
        SseParser p;
        const auto ev = p.feed("data: one\r\ndata: two\r\n\r\n");
        QCOMPARE(ev.size(), 1);
        QCOMPARE(ev[0].data, QByteArray("one\ntwo"));
    }
    void commentsAreIgnored()
    {
        SseParser p;
        const auto ev = p.feed(": keep-alive\n\ndata: x\n\n");
        QCOMPARE(ev.size(), 1);
        QCOMPARE(ev[0].data, QByteArray("x"));
    }
    void twoEventsInOneChunk()
    {
        SseParser p;
        QCOMPARE(p.feed("data: 1\n\ndata: 2\n\n").size(), 2);
    }
};

QTEST_APPLESS_MAIN(TstSseParser)
#include "tst_sseparser.moc"
