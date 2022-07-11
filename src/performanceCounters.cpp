#include "performanceCounters.hpp"

#include <atomic>
#include <mutex>

std::atomic<uint64_t> GlobalCounter::mutexLockCount = 0;
std::mutex GlobalCounter::dataMutex;
std::unordered_map<std::thread::id, Averages> GlobalCounter::threadAverages;

void
GlobalCounter::addMutexAquireCounters (const MutexAquireCounters &counters)
{
  ++mutexLockCount;
  std::unique_lock<std::mutex> lock (dataMutex);
  auto durationMicro =
    std::chrono::duration_cast<std::chrono::microseconds> (counters.endTimeAquire - counters.startTimeAquire);

  if (counters.lockType == LockType::READ)
    {
      auto currentAverage = threadAverages[counters.threadID].averageMicrosecondsRead;
      auto currentCounter = threadAverages[counters.threadID].readOperationCount;

      threadAverages[counters.threadID].averageMicrosecondsRead =
	(currentAverage * currentCounter + durationMicro.count ()) / (currentCounter + 1);
      threadAverages[counters.threadID].readOperationCount++;
    }
  else
    {
      auto currentAverage = threadAverages[counters.threadID].averageMicrosecondsWrite;
      auto currentCounter = threadAverages[counters.threadID].writeOperationCount;

      threadAverages[counters.threadID].averageMicrosecondsWrite =
	(currentAverage * currentCounter + durationMicro.count ()) / (currentCounter + 1);
      threadAverages[counters.threadID].writeOperationCount++;
    }
}