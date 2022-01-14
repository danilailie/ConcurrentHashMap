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

  Iterator (std::pair<KeyT, ValueT> *aKeyValue, Map const *const aMap, int aBucketIndex, int aValueIndex,
	    SharedLock aBucketLock, SharedLock aValueLock)
  {
    key = aKeyValue->first;
    map = aMap;
    keyValue = aKeyValue;

    bucketIndex = aBucketIndex;
    valueIndex = aValueIndex;
    bucketLock = aBucketLock;
    valueLock = aValueLock;

    increaseInstances ();
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

    increaseInstances ();
  }

  ~Iterator ()
  {
    decreaseInstances ();
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

  void
  increaseInstances ()
  {
    std::unique_lock<std::mutex> lock (instancesMutex);
    instances[map]++;
    if (instances[map] == 1)
      {
	locks.insert (std::make_pair (map, std::shared_lock<std::shared_mutex> (map->rehashMutex)));
      }
  }

  void
  decreaseInstances ()
  {
    std::unique_lock<std::mutex> lock (instancesMutex);
    instances[map]--;
    if (instances[map] == 0)
      {
	instances.erase (map);
	locks.erase (map);
      }
  }

private:
  using Bucket = bucket<KeyT, ValueT, HashFuncT>;
  using InternalValue = internal_value<KeyT, ValueT, HashFuncT>;

  KeyT key;
  const Map *map;

  std::pair<KeyT, ValueT> *keyValue;

  int bucketIndex;
  int valueIndex;
  SharedLock bucketLock;
  SharedLock valueLock;

  static std::unordered_map<const Map *, std::size_t> instances;
  static std::unordered_map<const Map *, std::shared_lock<std::shared_mutex>> locks;
  static std::mutex instancesMutex;

  friend Map;
  friend Bucket;
  friend InternalValue;
};

template <class KeyT, class ValueT, class HashFuncT>
std::unordered_map<const concurrent_unordered_map<KeyT, ValueT, HashFuncT> *, std::size_t>
  Iterator<KeyT, ValueT, HashFuncT>::instances;

template <class KeyT, class ValueT, class HashFuncT>
std::unordered_map<const concurrent_unordered_map<KeyT, ValueT, HashFuncT> *, std::shared_lock<std::shared_mutex>>
  Iterator<KeyT, ValueT, HashFuncT>::locks;

template <class KeyT, class ValueT, class HashFuncT> std::mutex Iterator<KeyT, ValueT, HashFuncT>::instancesMutex;

#endif