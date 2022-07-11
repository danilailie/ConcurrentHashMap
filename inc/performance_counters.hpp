#ifndef _PERFORMANCE_COUNTERS_HPP_
#define _PERFORMANCE_COUNTERS_HPP_

#include <atomic>
#include <chrono>
#include <mutex>
#include <thread>
#include <unordered_map>

#include "unordered_map_utils.hpp"

using ChronoTimePoint = std::chrono::time_point<std::chrono::steady_clock>;

struct MutexAquireCounters
{
  ChronoTimePoint startTimeAquire;
  ChronoTimePoint endTimeAquire;
  LockType lockType;
  std::thread::id threadID;
};

struct Averages
{
  uint64_t readOperationCount;
  double averageMicrosecondsRead;

  uint64_t writeOperationCount;
  double averageMicrosecondsWrite;
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

  static std::unordered_map<std::thread::id, Averages> &
  getAverages ()
  {
    return threadAverages;
  }

private:
  static std::atomic<uint64_t> mutexLockCount;
  static std::mutex dataMutex;

  static std::unordered_map<std::thread::id, Averages> threadAverages;
};

#endif