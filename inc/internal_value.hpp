#ifndef _INTERNAL_VALUE_HPP_
#define _INTERNAL_VALUE_HPP_

#include <shared_mutex>

template <class KeyT, class ValueT, class HashFuncT> class concurrent_unordered_map;

template <class KeyT, class ValueT, class HashFuncT> class internal_value
{
public:
  using Map = concurrent_unordered_map<KeyT, ValueT, HashFuncT>;
  using Iterator = typename concurrent_unordered_map<KeyT, ValueT, HashFuncT>::iterator;
  using SharedLock = std::shared_ptr<std::shared_lock<std::shared_mutex>>;

  internal_value (const KeyT &aKey, const ValueT &aValue) : isMarkedForDelete (false), keyValue (aKey, aValue)
  {
    valueMutex = std::make_unique<std::shared_mutex> ();
  }

  internal_value (const std::pair<KeyT, ValueT> &aKeyValuePair)
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

  Iterator
  getIterator (Map const *const aMap, int bucketIndex, int valueIndex, SharedLock bucketLock) const
  {
    auto valueLock = std::make_shared<std::shared_lock<std::shared_mutex>> (*valueMutex);
    auto keyValueP = const_cast<std::pair<KeyT, ValueT> *> (&keyValue);
    return Iterator (keyValueP, aMap, bucketIndex, valueIndex, bucketLock, valueLock);
  }

  void
  updateIterator (Iterator &it, int bucketIndex, int valueIndex) const
  {
    auto lock = std::make_shared<std::shared_lock<std::shared_mutex>> (*valueMutex);
    it.key = keyValue.first;
    it.bucketIndex = bucketIndex;
    it.valueIndex = valueIndex;
    it.valueLock = lock;
  }

  void
  updateValue (const ValueT &newValue)
  {
    std::unique_lock<std::shared_mutex> lock (*valueMutex);
    keyValue.second = newValue;
  }

  bool
  lock () const
  {
    return valueMutex->try_lock_shared ();
  }

  void
  unlock () const
  {
    return valueMutex->unlock_shared ();
  }

private:
  std::unique_ptr<std::shared_mutex> valueMutex;
  bool isMarkedForDelete;
  std::pair<KeyT, ValueT> keyValue;

  friend Map;
};

#endif
