#include "performanceCounters.hpp"

std::atomic<uint64_t> GlobalCounter::mutexLockCount = 0;

void
GlobalCounter::addMutexAquireCounters (const MutexAquireCounters &counters)
{
  ++mutexLockCount;
}