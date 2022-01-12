#ifndef _FORWARD_ITERATOR_HPP_
#define _FORWARD_ITERATOR_HPP_

template <class KeyT, class ValueT, class HashFuncT> class concurrent_unordered_map;
template <class KeyT, class ValueT, class HashFuncT> class bucket;
template <class KeyT, class ValueT, class HashFuncT> class internal_value;

template <class KeyT, class ValueT, class HashFuncT> class iterator
{
public:
  using Map = concurrent_unordered_map<KeyT, ValueT, HashFuncT>;
  using SharedLock = std::shared_ptr<std::shared_lock<std::shared_mutex>>;

  iterator (const KeyT &aKey, Map const *const aMap, int aBucketIndex, int aValueIndex, SharedLock aBucketLock,
		    SharedLock aValueLock)
  {
    key = aKey;
    map = aMap;
    bucketIndex = aBucketIndex;
    valueIndex = aValueIndex;
    bucketLock = aBucketLock;
    valueLock = aValueLock;

    increaseInstances ();
  }

  iterator (const iterator &other)
  {
    map = other.map;
    bucketIndex = other.bucketIndex;
    valueIndex = other.valueIndex;
    valueLock = other.valueLock;

    increaseInstances ();
  }

  ~iterator ()
  {
    decreaseInstances ();
  }

  iterator &
  operator= (const iterator &other)
  {
    if (this == &other)
      {
	return *this;
      }

    map = other.map;
    key = other.key;
    bucketIndex = other.bucketIndex;
    valueIndex = other.valueIndex;
    valueLock = other.valueLock;

    return *this;
  }

  std::pair<KeyT, ValueT> &
  operator* ()
  {
    return map->getIterValue (*this);
  }

  std::pair<KeyT, ValueT> *
  operator-> ()
  {
    return map->getIterPtr (*this);
  }

  bool
  operator== (const iterator &another)
  {
    return map == another.map && key == another.key;
  }

  bool
  operator!= (const iterator &other)
  {
    return map != other.map || key != other.key;
  }

  iterator &
  operator++ ()
  {
    if (!bucketLock)
      {
	bucketLock = map->aquireBucketLock (bucketIndex);
      }
    map->advanceIterator (*this);
    return *this;
  }

  iterator &
  operator++ (int)
  {
    forward_iterator tmp = *this;
    ++(*this);
    return tmp;
  }

private:
  iterator (const KeyT &aKey, Map const *const aMap, int aBucketIndex, int aValueIndex)
  {
    key = aKey;
    map = aMap;
    bucketIndex = aBucketIndex;
    valueIndex = aValueIndex;

    increaseInstances ();
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
  iterator<KeyT, ValueT, HashFuncT>::instances;

template <class KeyT, class ValueT, class HashFuncT>
std::unordered_map<const concurrent_unordered_map<KeyT, ValueT, HashFuncT> *, std::shared_lock<std::shared_mutex>>
  iterator<KeyT, ValueT, HashFuncT>::locks;

template <class KeyT, class ValueT, class HashFuncT>
std::mutex iterator<KeyT, ValueT, HashFuncT>::instancesMutex;

#endif