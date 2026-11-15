#include "opencodeclient.h"
#include <QJsonDocument>
#include <QtTest>

class TstClientParse : public QObject {
    Q_OBJECT
private slots:
    void v1MessageListKeepsOnlyTextAfterLastUser()
    {
        const auto doc = QJsonDocument::fromJson(R"([
          {"info":{"role":"user"},"parts":[{"type":"text","text":"old q"}]},
          {"info":{"role":"assistant"},"parts":[{"type":"text","text":"old a"}]},
          {"info":{"role":"user"},"parts":[{"type":"text","text":"new q"}]},
          {"info":{"role":"assistant"},"parts":[{"type":"tool"},{"type":"text","text":"part one"},{"type":"text","text":"part two"}]}
        ])");
        QCOMPARE(OpenCodeClient::assistantTextAfterLastUser(OpenCodeClient::extractArray(doc)), QString("part one\n\npart two"));
    }
    void v2WrappedListWithTypeField()
    {
        const auto doc = QJsonDocument::fromJson(R"({"data":[
          {"type":"user","content":[{"type":"text","text":"q"}]},
          {"type":"assistant","content":[{"type":"text","text":"answer"}]}
        ]})");
        const QJsonArray arr = OpenCodeClient::extractArray(doc);
        QCOMPARE(arr.size(), 2);
        QCOMPARE(OpenCodeClient::assistantTextAfterLastUser(arr), QString("answer"));
    }
    void ignoresSyntheticAndOtherTypes()
    {
        const auto doc = QJsonDocument::fromJson(R"([
          {"type":"user","content":[{"type":"text","text":"q"}]},
          {"type":"synthetic","content":[{"type":"text","text":"hidden"}]},
          {"type":"assistant","content":[{"type":"text","text":"ok","synthetic":false}]}
        ])");
        QCOMPARE(OpenCodeClient::assistantTextAfterLastUser(OpenCodeClient::extractArray(doc)), QString("ok"));
    }
};

QTEST_GUILESS_MAIN(TstClientParse)
#include "tst_clientparse.moc"
