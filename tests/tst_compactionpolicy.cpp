#include "compactionpolicy.h"

#include <QtTest>

// F5: the automatic compaction rules, as pure decisions.
class TstCompactionPolicy : public QObject {
    Q_OBJECT
private:
    // A state where every rule is satisfied: the answer is "compact now".
    static CompactionState readyState()
    {
        CompactionState s;
        s.enabled = true;
        s.contextPercent = 85;
        s.thresholdPercent = 80;
        return s;
    }

private slots:
    void compactsWhenAboveThreshold()
    {
        QVERIFY(CompactionPolicy::shouldCompact(readyState()));
    }

    void waitsBelowThreshold()
    {
        CompactionState s = readyState();
        s.contextPercent = 79;
        QVERIFY(!CompactionPolicy::shouldCompact(s));
        s.contextPercent = 80;                       // the threshold itself counts
        QVERIFY(CompactionPolicy::shouldCompact(s));
    }

    void unknownUsageOrDisabledNeverCompacts()
    {
        CompactionState s = readyState();
        s.contextPercent = -1;
        QVERIFY(!CompactionPolicy::shouldCompact(s));
        s = readyState();
        s.enabled = false;
        QVERIFY(!CompactionPolicy::shouldCompact(s));
    }

    void neverDuringATurnOrWhileLoadingOrInFlight()
    {
        CompactionState s = readyState();
        s.busy = true;
        QVERIFY(!CompactionPolicy::shouldCompact(s));
        s = readyState();
        s.compactionInFlight = true;
        QVERIFY(!CompactionPolicy::shouldCompact(s));
    }

    void yieldsToTheServerWhenItCompactsItself()
    {
        CompactionState s = readyState();
        s.serverCompactsItself = true;
        QVERIFY(!CompactionPolicy::shouldCompact(s));
    }

    void cooldownSeparatesRequests()
    {
        CompactionState s = readyState();
        s.msSinceLastRequest = 10 * 1000;
        QVERIFY(!CompactionPolicy::shouldCompact(s));
        s.msSinceLastRequest = CompactionPolicy::kCooldownMs;
        QVERIFY(CompactionPolicy::shouldCompact(s));
    }

    void failuresStopAfterTheLimit()
    {
        CompactionState s = readyState();
        s.failures = CompactionPolicy::kMaxFailures - 1;
        QVERIFY(CompactionPolicy::shouldCompact(s));
        s.failures = CompactionPolicy::kMaxFailures;
        QVERIFY(!CompactionPolicy::shouldCompact(s));
    }

    void noRepeatWithoutANewTurn()
    {
        // A summary that does not free enough space must not trigger another compaction with nothing new.
        CompactionState s = readyState();
        s.turnsSinceLastCompaction = 0;
        QVERIFY(!CompactionPolicy::shouldCompact(s));
        s.turnsSinceLastCompaction = 1;
        QVERIFY(CompactionPolicy::shouldCompact(s));
    }
};

QTEST_GUILESS_MAIN(TstCompactionPolicy)
#include "tst_compactionpolicy.moc"
