#ifndef _HASH_MAP_UTILS_HPP_
#define _HASH_MAP_UTILS_HPP_

#include <atomic>
#include <map>
#include <memory>
#include <mutex>
#include <shared_mutex>
#include <type_traits>
#include <variant>
#include <vector>

using ReadLock = std::shared_lock<std::shared_mutex>;
using SharedReadLock = std::shared_ptr<ReadLock>;
using WriteLock = std::unique_lock<std::shared_mutex>;
using SharedWriteLock = std::shared_ptr<WriteLock>;
using VariantLock = std::variant<SharedReadLock, SharedWriteLock>;
using SharedVariantLock = std::shared_ptr<VariantLock>;
using WeakVariantLock = std::weak_ptr<VariantLock>;

enum class LockType
{
  READ = 0,
  WRITE
};

using LockMap = std::map<std::shared_mutex *, std::tuple<WeakVariantLock, LockType>>;

static uint64_t
getNextPrimeNumber (const uint64_t &currentNumber)
{
  std::vector<std::uint64_t> primeNumbers = { 41,	83,	   167,	      337,	 677,	    1361,     2729,
					      5471,	10949,	   21911,     43853,	 87719,	    175447,   350899,
					      701819,	1403641,   2807303,   5614657,	 11229331,  22458671, 44917381,
					      89834777, 179669557, 359339171, 718678369, 1437356741 };
  auto findResult = std::lower_bound (primeNumbers.begin (), primeNumbers.end (), currentNumber);

  if (*findResult == currentNumber)
    {
      ++findResult;
    }

  return *findResult;

  return 1437356741;
}

#endif
