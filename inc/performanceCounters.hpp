#ifndef _PERFORMANCE_COUNTERS_HPP_
#define _PERFORMANCE_COUNTERS_HPP_

#include <chrono>
#include <thread>

#include "unordered_map_utils.hpp"

using ChronoTimePoint = std::chrono::time_point<std::chrono::steady_clock>;

struct MutexAquireCounters
{
  ChronoTimePoint startTimeAquire;
  ChronoTimePoint endTimeAquire;
  LockType lockType;
  std::thread::id threadID;
};

class GlobalCounter
{
public:
  GlobalCounter () = delete;
  static void addMutexAquireCounters (const MutexAquireCounters &counters);
  static uint64_t
  getLockCount ()
  {
    return mutexLockCount;
  }

private:
  static std::atomic<uint64_t> mutexLockCount;
};

#endif