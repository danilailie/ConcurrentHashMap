#ifndef _INTERNAL_VALUE_HPP_
#define _INTERNAL_VALUE_HPP_

#include <shared_mutex>

template <class KeyT, class ValueT, class HashFuncT> class ConcurrentHashMap;

template <class KeyT, class ValueT, class HashFuncT> class InternalValueType
{
public:
  InternalValueType (const KeyT &aKey, const ValueT &aValue) : isMarkedForDelete (false), key (aKey), userValue (aValue)
  {
    valueMutex = std::make_unique<std::shared_mutex> ();
  }

  InternalValueType (const std::pair<KeyT, ValueT> &aKeyValuePair)
      : isMarkedForDelete (false), key (aKeyValuePair.first), userValue (aKeyValuePair.second)
  {
    valueMutex = std::make_unique<std::shared_mutex> ();
  }

  bool
  compareKey (const KeyT &aKey) const
  {
    std::shared_lock<std::shared_mutex> lock (*valueMutex);
    if (!isMarkedForDelete)
      {
	return key == aKey;
      }
    return false;
  }

  std::pair<KeyT, ValueT>
  getKeyValuePair ()
  {
    std::shared_lock<std::shared_mutex> lock (*valueMutex);
    return std::make_pair (key, userValue);
  }

  void
  erase ()
  {
    std::unique_lock<std::shared_mutex> lock (*valueMutex);
    isMarkedForDelete = true;
  }

  bool
  isAvailable () const
  {
    std::shared_lock<std::shared_mutex> lock (*valueMutex);
    return !isMarkedForDelete;
  }

  void
  setAvailable ()
  {
    std::unique_lock<std::shared_mutex> lock (*valueMutex);
    isMarkedForDelete = false;
  }

  KeyT
  getKey () const
  {
    std::shared_lock<std::shared_mutex> lock (*valueMutex);
    return key;
  }

private:
  using Map = ConcurrentHashMap<KeyT, ValueT, HashFuncT>;

  std::unique_ptr<std::shared_mutex> valueMutex;
  bool isMarkedForDelete;
  KeyT key;
  ValueT userValue;

  friend Map;
};

#endif
