#ifndef _INTERNAL_VALUE_HPP_
#define _INTERNAL_VALUE_HPP_

#include <shared_mutex>

template <class KeyT, class ValueT, class HashFuncT> class ConcurrentHashMap;

template <class KeyT, class ValueT, class HashFuncT> class InternalValueType
{
public:
  InternalValueType (const KeyT &aKey, const ValueT &aValue) : isMarkedForDelete (false), keyValue (aKey, aValue)
  {
    valueMutex = std::make_unique<std::shared_mutex> ();
  }

  InternalValueType (const std::pair<KeyT, ValueT> &aKeyValuePair)
      : isMarkedForDelete (false), keyValue (aKeyValuePair.first, aKeyValuePair.second)
  {
    valueMutex = std::make_unique<std::shared_mutex> ();
  }

  bool
  compareKey (const KeyT &aKey) const
  {
    std::shared_lock<std::shared_mutex> lock (*valueMutex);
    if (!isMarkedForDelete)
      {
	return keyValue.first == aKey;
      }
    return false;
  }

  std::pair<KeyT, ValueT>
  getKeyValuePair () const
  {
    std::shared_lock<std::shared_mutex> lock (*valueMutex);
    return keyValue;
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
    return keyValue.first;
  }

private:
  using Map = ConcurrentHashMap<KeyT, ValueT, HashFuncT>;

  std::unique_ptr<std::shared_mutex> valueMutex;
  bool isMarkedForDelete;
  std::pair<KeyT, ValueT> keyValue;

  friend Map;
};

#endif
