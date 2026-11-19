#include "compactionpolicy.h"

bool CompactionPolicy::shouldCompact(const CompactionState &s)
{
    if (!s.enabled || s.contextPercent < 0)
        return false;
    if (s.contextPercent < s.thresholdPercent)
        return false;
    if (s.busy || s.compactionInFlight)
        return false;
    if (s.serverCompactsItself)
        return false;
    if (s.failures >= kMaxFailures)
        return false;
    if (s.msSinceLastRequest >= 0 && s.msSinceLastRequest < kCooldownMs)
        return false;
    if (s.turnsSinceLastCompaction == 0)
        return false;
    return true;
}
