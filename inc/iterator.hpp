#ifndef _FORWARD_ITERATOR_HPP_
#define _FORWARD_ITERATOR_HPP_

#include <variant>

#include "unordered_map_utils.hpp"

template <class KeyT, class ValueT, class HashFuncT> class concurrent_unordered_map;
template <class KeyT, class ValueT, class HashFuncT> class bucket;
template <class KeyT, class ValueT, class HashFuncT> class internal_value;

template <class KeyT, class ValueT, class HashFuncT> class Iterator
{
public:
  using Map = concurrent_unordered_map<KeyT, ValueT, HashFuncT>;
  using SharedLock = std::shared_ptr<std::shared_lock<std::shared_mutex>>;
  using WriteLock = std::shared_ptr<std::unique_lock<std::shared_mutex>>;
  using VariandLock = std::variant<SharedLock, WriteLock>;

  Iterator (std::pair<KeyT, ValueT> *aKeyValue, Map const *const aMap, int aBucketIndex, int aValueIndex,
	    VariandLock aBucketLock, VariandLock aValueLock)
  {
    key = aKeyValue->first;
    map = aMap;
    keyValue = aKeyValue;

    bucketIndex = aBucketIndex;
    valueIndex = aValueIndex;
    bucketLock = aBucketLock;
    valueLock = aValueLock;
    valueLockType = isWriteLocked () ? ValueLockType::WRITE : ValueLockType::READ;
  }

  Iterator (const Iterator &other)
  {
    key = other.key;
    map = other.map;
    keyValue = other.keyValue;

    bucketIndex = other.bucketIndex;
    valueIndex = other.valueIndex;
    bucketLock = other.bucketLock;
    valueLock = other.valueLock;
    valueLockType = other.valueLockType;
  }

  ~Iterator ()
  {
  }

  Iterator &
  operator= (const Iterator &other)
  {
    if (this == &other)
      {
	return *this;
      }

    map = other.map;
    key = other.key;
    keyValue = other.keyValue;
    bucketIndex = other.bucketIndex;
    valueIndex = other.valueIndex;
    valueLock = other.valueLock;
    valueLockType = other.valueLockType;

    return *this;
  }

  std::pair<KeyT, ValueT> &
  operator* ()
  {
    return *keyValue;
  }

  std::pair<KeyT, ValueT> *
  operator-> ()
  {
    return keyValue;
  }

  bool
  operator== (const Iterator &another) const
  {
    return map == another.map && key == another.key;
  }

  bool
  operator!= (const Iterator &other) const
  {
    return map != other.map || key != other.key;
  }

  Iterator &
  operator++ ()
  {
    bool hasBucketLock = false;
    if (std::holds_alternative<SharedLock> (bucketLock))
      {
	auto lockPtr = std::get<SharedLock> (bucketLock);
	if (lockPtr)
	  hasBucketLock = true;
      }
    else
      {
	auto lockPtr = std::get<WriteLock> (bucketLock);
	if (lockPtr)
	  hasBucketLock = true;
      }

    if (!hasBucketLock)
      {
	bucketLock = map->aquireBucketLock (bucketIndex);
      }
    map->advanceIterator (*this);
    return *this;
  }

  Iterator &
  operator++ (int)
  {
    Iterator tmp = *this;
    ++(*this);
    return tmp;
  }

private:
  Iterator (const KeyT &aKey, Map const *const aMap, int aBucketIndex, int aValueIndex) : keyValue (nullptr)
  {
    key = aKey;
    map = aMap;
    bucketIndex = aBucketIndex;
    valueIndex = aValueIndex;
  }

  bool
  isWriteLocked () const
  {
    return std::get_if<WriteLock> (&valueLock);
  }

private:
  using Bucket = bucket<KeyT, ValueT, HashFuncT>;
  using InternalValue = internal_value<KeyT, ValueT, HashFuncT>;

  KeyT key;
  const Map *map;

  std::pair<KeyT, ValueT> *keyValue;

  int bucketIndex;
  int valueIndex;
  VariandLock bucketLock;
  VariandLock valueLock;
  ValueLockType valueLockType;

  friend Map;
  friend Bucket;
  friend InternalValue;
};

#endif
