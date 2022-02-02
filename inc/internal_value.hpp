#ifndef _INTERNAL_VALUE_HPP_
#define _INTERNAL_VALUE_HPP_

#include <optional>
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

  internal_value (const internal_value &other)
      : isMarkedForDelete (other.isMarkedForDelete)
      , keyValue (other.keyValue)
      , iteratorCount (other.iteratorCount.load ())
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

  std::optional<KeyT>
  getKey () const
  {
    std::shared_lock<std::shared_mutex> lock (*valueMutex);
    if (!isMarkedForDelete)
      {
	return keyValue.first;
      }
    return {};
  }

  Iterator
  getIterator (Map const *const aMap, int bucketIndex, int valueIndex, SharedLock bucketLock) const
  {
    auto valueLock = std::make_unique<std::shared_lock<std::shared_mutex>> (*valueMutex);

    auto self = const_cast<internal_value<KeyT, ValueT, HashFuncT> *> (this);
    return Iterator (self, aMap, bucketIndex, valueIndex, bucketLock);
  }

  std::optional<Iterator>
  getIteratorForKey (Map const *const aMap, KeyT key, int bucketIndex, int valueIndex, SharedLock bucketLock) const
  {
    auto valueLock = std::make_unique<std::shared_lock<std::shared_mutex>> (*valueMutex);

    if (!isMarkedForDelete && keyValue.first == key)
      {
	auto self = const_cast<internal_value<KeyT, ValueT, HashFuncT> *> (this);
	return Iterator (self, aMap, bucketIndex, valueIndex, bucketLock);
      }

    return {};
  }

  void
  updateIterator (Iterator &it, int bucketIndex, int valueIndex, SharedLock bucketLock) const
  {
    auto valueLock = std::make_unique<std::shared_lock<std::shared_mutex>> (*valueMutex);
    it.internalValue->iteratorCount--;
    it.internalValue = const_cast<internal_value<KeyT, ValueT, HashFuncT> *> (this);
    it.key = keyValue.first;
    it.bucketIndex = bucketIndex;
    it.valueIndex = valueIndex;
    it.bucketLock = bucketLock;
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
  std::atomic<std::size_t> iteratorCount = 0;

  friend Map;
  friend Iterator;
};

#endif
