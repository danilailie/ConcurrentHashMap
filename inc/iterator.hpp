#ifndef _FORWARD_ITERATOR_HPP_
#define _FORWARD_ITERATOR_HPP_

#include <variant>

#include "internal_value.hpp"
#include "unordered_map_utils.hpp"

template <class KeyT, class ValueT, class HashFuncT> class concurrent_unordered_map;
template <class KeyT, class ValueT, class HashFuncT> class bucket;
template <class KeyT, class ValueT, class HashFuncT> class internal_value;

template <class KeyT, class ValueT, class HashFuncT> class Iterator
{
public:
  using Map = concurrent_unordered_map<KeyT, ValueT, HashFuncT>;
  using InternalValue = internal_value<KeyT, ValueT, HashFuncT>;

  Iterator (std::shared_ptr<const InternalValue> value, Map const *const aMap, int aBucketIndex, int aValueIndex,
	    SharedVariantLock aBucketLock, SharedVariantLock aValueLock)
  {
    key = value->keyValue.first;
    internalValue = value;
    map = aMap;

    bucketIndex = aBucketIndex;
    valueIndex = aValueIndex;
    bucketLock = aBucketLock;
    valueLock = aValueLock;
  }

  Iterator (const Iterator &other)
  {
    key = other.key;
    internalValue = other.internalValue;
    map = other.map;

    bucketIndex = other.bucketIndex;
    valueIndex = other.valueIndex;
    bucketLock = other.bucketLock;
    valueLock = other.valueLock;
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
    internalValue = other.internalValue;
    key = other.key;
    bucketIndex = other.bucketIndex;
    valueIndex = other.valueIndex;
    valueLock = other.valueLock;

    return *this;
  }

  std::pair<KeyT, ValueT> &
  operator* () const
  {
    std::pair<KeyT, ValueT> *keyValueP = const_cast<std::pair<KeyT, ValueT> *> (&(internalValue->keyValue));
    return *keyValueP;
  }

  std::pair<KeyT, ValueT> *
  operator-> () const
  {
    std::pair<KeyT, ValueT> *keyValueP = const_cast<std::pair<KeyT, ValueT> *> (&(internalValue->keyValue));
    return keyValueP;
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
    bool hasBucketLock = bucketLock != nullptr;

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
  Iterator (const KeyT &aKey, Map const *const aMap, int aBucketIndex, int aValueIndex)
  {
    internalValue = std::shared_ptr<InternalValue> ();
    key = aKey;
    map = aMap;
    bucketIndex = aBucketIndex;
    valueIndex = aValueIndex;
  }

private:
  using Bucket = bucket<KeyT, ValueT, HashFuncT>;

  KeyT key;
  const Map *map;

  std::shared_ptr<const InternalValue> internalValue;

  int bucketIndex;
  int valueIndex;
  SharedVariantLock bucketLock;
  SharedVariantLock valueLock;

  friend Map;
  friend Bucket;
  friend InternalValue;
};

#endif
