#ifndef _FORWARD_ITERATOR_HPP_
#define _FORWARD_ITERATOR_HPP_

template <class KeyT, class ValueT, class HashFuncT> class concurrent_unordered_map;
template <class KeyT, class ValueT, class HashFuncT> class bucket;
template <class KeyT, class ValueT, class HashFuncT> class internal_value;

template <class KeyT, class ValueT, class HashFuncT> class Iterator
{
public:
  using Map = concurrent_unordered_map<KeyT, ValueT, HashFuncT>;
  using SharedLock = std::shared_ptr<std::shared_lock<std::shared_mutex>>;
  using InternalValue = internal_value<KeyT, ValueT, HashFuncT>;

  Iterator (InternalValue *anInternalValue, Map const *const aMap, int aBucketIndex, int aValueIndex,
	    SharedLock aBucketLock)
  {
    ++anInternalValue->iteratorCount;
    key = anInternalValue->keyValue.first;
    map = aMap;
    internalValue = anInternalValue;

    bucketIndex = aBucketIndex;
    valueIndex = aValueIndex;
    bucketLock = aBucketLock;
  }

  Iterator (const Iterator &other)
  {
    key = other.key;
    map = other.map;
    internalValue = other.internalValue;
    ++internalValue->iteratorCount;

    bucketIndex = other.bucketIndex;
    valueIndex = other.valueIndex;
    bucketLock = other.bucketLock;
  }

  ~Iterator ()
  {
    if (key != InvalidKeyValue<KeyT> ())
      {
	--(internalValue->iteratorCount);
      }
  }

  Iterator &
  operator= (const Iterator &other)
  {
    if (this == &other)
      {
	return *this;
      }

    --(internalValue->iteratorCount);

    map = other.map;
    key = other.key;

    if (key != InvalidKeyValue<KeyT> ())
      {
	internalValue = other.internalValue;
	++(internalValue->iteratorCount);
      }
    bucketIndex = other.bucketIndex;
    valueIndex = other.valueIndex;

    return *this;
  }

  std::pair<KeyT, ValueT> &
  operator* ()
  {
    return internalValue->keyValue;
  }

  std::pair<KeyT, ValueT> *
  operator-> ()
  {
    return &(internalValue->keyValue);
  }

  bool
  operator== (const Iterator &another)
  {
    return map == another.map && key == another.key;
  }

  bool
  operator!= (const Iterator &other)
  {
    return map != other.map || key != other.key;
  }

  Iterator &
  operator++ ()
  {
    if (!bucketLock)
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
    key = aKey;
    map = aMap;
    bucketIndex = aBucketIndex;
    valueIndex = aValueIndex;
  }

private:
  using Bucket = bucket<KeyT, ValueT, HashFuncT>;

  KeyT key;
  const Map *map;

  InternalValue *internalValue;

  int bucketIndex;
  int valueIndex;
  SharedLock bucketLock;

  friend Map;
  friend Bucket;
  friend InternalValue;
};

#endif
