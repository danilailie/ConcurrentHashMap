#ifndef _HASH_MAP_UTILS_HPP_
#define _HASH_MAP_UTILS_HPP_

#include <memory>
#include <type_traits>

using ReadLock = std::shared_lock<std::shared_mutex>;
using SharedReadLock = std::shared_ptr<ReadLock>;
using WriteLock = std::unique_lock<std::shared_mutex>;
using SharedWriteLock = std::shared_ptr<WriteLock>;
using VariantLock = std::variant<SharedReadLock, SharedWriteLock>;
using SharedVariantLock = std::shared_ptr<VariantLock>;
using WeakVariantLock = std::weak_ptr<VariantLock>;

template <typename KeyT, std::enable_if_t<std::is_integral<KeyT>::value, bool> = true>
KeyT
InvalidKeyValue ()
{
  return -1;
}

template <typename KeyT, std::enable_if_t<!std::is_integral<KeyT>::value, bool> = true>
KeyT
InvalidKeyValue ()
{
  return KeyT ();
}

enum class ValueLockType
{
  READ = 0,
  WRITE
};

uint64_t
getNextPrimeNumber (const uint64_t &currentNumber)
{
  std::vector<std::uint64_t> primeNumbers = { 41,	83,	   167,	      337,	 677,	    1361,     2729,
					      5471,	10949,	   21911,     43853,	 87719,	    175447,   350899,
					      701819,	1403641,   2807303,   5614657,	 11229331,  22458671, 44917381,
					      89834777, 179669557, 359339171, 718678369, 1437356741 };
  return 0;
}

#endif
