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

#endif
