#pragma once
#include <QtGlobal>

// F5 automatic context management: the pure decision "should Qute ask opencode to compact the session now?".
// No Qt objects, no network: SessionStore fills in the state and acts on the answer, so the rules are unit tested.
//
// Rules (all must hold):
//  - auto compaction is enabled in settings and the context usage is known
//  - usage has reached the threshold (settings, default 80 %)
//  - no turn is running, history is not loading, and no compaction is already in flight
//  - the server does not compact on its own for this session (then opencode does it and Qute stays out of the way)
//  - fewer than kMaxFailures compactions failed in a row (the user's next prompt resets the count)
//  - at least kCooldownMs since our last request
//  - at least one prompt was sent since the last compaction, so a summary that does not free enough space
//    cannot trigger another compaction with nothing new in the session
struct CompactionState {
    bool enabled = true;
    int contextPercent = -1;            // -1 = unknown
    int thresholdPercent = 80;
    bool busy = false;                  // a turn is running or the history is loading
    bool compactionInFlight = false;    // our request is on its way, or a compaction is running
    bool serverCompactsItself = false;  // the server compacted without a request from us
    int failures = 0;                   // compactions that failed in a row
    qint64 msSinceLastRequest = -1;     // -1 = never requested
    int turnsSinceLastCompaction = -1;  // -1 = never compacted; prompts sent since then
};

class CompactionPolicy {
public:
    static constexpr qint64 kCooldownMs = 60 * 1000;
    static constexpr int kMaxFailures = 3;

    static bool shouldCompact(const CompactionState &s);
};
