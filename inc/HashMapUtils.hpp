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

std::size_t
getNextPrimeNumber (const std::size_t &aValue)
{
  std::vector<std::size_t> primes = { 2,  3,  5,  7,  11, 13, 17, 19, 23, 29, 31, 37,	 41,	43,
				      47, 53, 59, 61, 67, 71, 73, 79, 83, 89, 97, 10007, 20021, 40063 };
  for (std::size_t i = 0; i < primes.size (); ++i)
    {
      if (primes[i] > aValue)
	{
	  return primes[i];
	}
    }
  return 10007;
}

#endif