#ifndef _INTERNAL_VALUE_HPP_
#define _INTERNAL_VALUE_HPP_

#include <shared_mutex>

template <class KeyT, class ValueT, class HashFuncT> class InternalValue
{
public:
  InternalValue (const KeyT &aKey, const ValueT &aValue) : isMarkedForDelete (false), key (aKey), userValue (aValue)
  {
    valueMutex = std::make_unique<std::shared_mutex> ();
  }

  bool
  compareKey (const KeyT &aKey)
  {
    std::shared_lock<std::shared_mutex> lock (*valueMutex);
    return key == aKey;
  }

  std::pair<KeyT, ValueT>
  getKeyValuePair ()
  {
    std::shared_lock<std::shared_mutex> lock (*valueMutex);
    return std::make_pair (key, userValue);
  }

private:
  std::unique_ptr<std::shared_mutex> valueMutex;
  bool isMarkedForDelete;
  KeyT key;
  ValueT userValue;
};

#endif
