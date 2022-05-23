#ifndef _HASH_MAP_UTILS_HPP_
#define _HASH_MAP_UTILS_HPP_

#include <type_traits>

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
  return "";
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
}

#endif
