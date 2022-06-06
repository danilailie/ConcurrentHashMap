#ifndef _INTERNAL_VALUE_HPP_
#define _INTERNAL_VALUE_HPP_

#include <optional>
#include <shared_mutex>
#include <variant>

#include "unordered_map_utils.hpp"

template <class KeyT, class ValueT, class HashFuncT> class concurrent_unordered_map;

template <class KeyT, class ValueT, class HashFuncT>
class internal_value : public std::enable_shared_from_this<internal_value<KeyT, ValueT, HashFuncT>>
{
public:
  using Map = concurrent_unordered_map<KeyT, ValueT, HashFuncT>;
  using Iterator = typename concurrent_unordered_map<KeyT, ValueT, HashFuncT>::iterator;

  internal_value (const KeyT &aKey, const ValueT &aValue) : isMarkedForDelete (false), keyValue (aKey, aValue)
  {
    valueMutex = std::make_unique<std::shared_mutex> ();
  }

  internal_value (const std::pair<KeyT, ValueT> &aKeyValuePair) : isMarkedForDelete (false), keyValue (aKeyValuePair)
  {
    valueMutex = std::make_unique<std::shared_mutex> ();
  }

  bool
  compareKey (const KeyT &aKey) const
  {
    auto valueLock = Map::getValueLockFor (&(*valueMutex), ValueLockType::READ);
    if (!isMarkedForDelete)
      {
	return keyValue.first == aKey;
      }
    return false;
  }

  std::pair<KeyT, ValueT>
  getKeyValuePair () const
  {
    auto valueLock = Map::getValueLockFor (&(*valueMutex), ValueLockType::READ);
    return keyValue;
  }

  void
  erase ()
  {
    auto valueLock = Map::getValueLockFor (&(*valueMutex), ValueLockType::WRITE);
    isMarkedForDelete = true;
  }

  bool
  isAvailable () const
  {
    auto valueLock = Map::getValueLockFor (&(*valueMutex), ValueLockType::READ);
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
    auto valueLock = Map::getValueLockFor (&(*valueMutex), ValueLockType::READ);
    if (!isMarkedForDelete)
      {
	return keyValue.first;
      }
    return {};
  }

  Iterator
  getIterator (Map const *const aMap, int bucketIndex, int valueIndex, SharedVariantLock bucketLock,
	       bool isWriteValueLocked = false) const
  {
    SharedVariantLock valueLock;
    if (isWriteValueLocked)
      {
	valueLock = Map::getValueLockFor (&(*valueMutex), ValueLockType::WRITE);
      }
    else
      {
	valueLock = Map::getValueLockFor (&(*valueMutex), ValueLockType::READ);
      }

    auto self = this->shared_from_this ();
    return Iterator (self, aMap, bucketIndex, valueIndex, bucketLock, valueLock);
  }

  std::optional<Iterator>
  getIteratorForKey (Map const *const aMap, KeyT key, int bucketIndex, int valueIndex, SharedVariantLock bucketLock,
		     ValueLockType valueLockType) const
  {
    auto valueLock = Map::getValueLockFor (&(*valueMutex), valueLockType);

    if (!isMarkedForDelete && keyValue.first == key)
      {
	auto self = this->shared_from_this ();
	return Iterator (self, aMap, bucketIndex, valueIndex, bucketLock, valueLock);
      }

    return {};
  }

  void
  updateIterator (Iterator &it, int bucketIndex, int valueIndex, SharedVariantLock bucketLock) const
  {
    auto valueLock = Map::getValueLockFor (&(*valueMutex), ValueLockType::READ);

    it.internalValue = this->shared_from_this ();
    it.key = keyValue.first;
    it.bucketIndex = bucketIndex;
    it.valueIndex = valueIndex;
    it.valueLock = valueLock;
    it.bucketLock = bucketLock;
  }

  void
  updateValue (const ValueT &newValue)
  {
    std::unique_lock<std::shared_mutex> lock (*valueMutex);
    keyValue.second = newValue;
  }

private:
  std::unique_ptr<std::shared_mutex> valueMutex;
  bool isMarkedForDelete;
  std::pair<KeyT, ValueT> keyValue;

  friend Map;
  friend Iterator;
};

#endif
