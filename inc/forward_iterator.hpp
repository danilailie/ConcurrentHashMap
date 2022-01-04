#ifndef _FORWARD_ITERATOR_HPP_
#define _FORWARD_ITERATOR_HPP_

template <class KeyT, class ValueT, class HashFuncT> class concurrent_unordered_map;
template <class KeyT, class ValueT, class HashFuncT> class bucket;
template <class KeyT, class ValueT, class HashFuncT> class internal_value;

template <class KeyT, class ValueT, class HashFuncT> class forward_iterator
{
public:
  using Map = concurrent_unordered_map<KeyT, ValueT, HashFuncT>;
  using UniqueSharedLock = std::shared_ptr<std::shared_lock<std::shared_mutex>>;

  forward_iterator (const KeyT &aKey, Map const *const aMap, std::size_t aBucketIndex, int aValueIndex,
		    UniqueSharedLock &lock)
  {
    key = aKey;
    map = aMap;
    bucketIndex = aBucketIndex;
    valueIndex = aValueIndex;
    valueLock = lock;

    increaseInstances ();
  }

  forward_iterator (const forward_iterator &other)
  {
    map = other.map;
    bucketIndex = other.bucketIndex;
    valueIndex = other.valueIndex;
    valueLock = other.valueLock;

    increaseInstances ();
  }

  ~forward_iterator ()
  {
    decreaseInstances ();
  }

  forward_iterator &
  operator= (const forward_iterator &other)
  {
    if (this == &other)
      {
	return *this;
      }

    map = other.map;
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
  operator== (const forward_iterator &another)
  {
    return map == another.map && key == another.key;
  }

  bool
  operator!= (const forward_iterator &other)
  {
    return map != other.map || key != other.key;
  }

  forward_iterator &
  operator++ ()
  {
    //     key = map->getNextElement (bucketIndex, valueIndex);
    map->advanceIterator (*this);
    return *this;
  }

  forward_iterator &
  operator++ (int)
  {
    forward_iterator tmp = *this;
    ++(*this);
    return tmp;
  }

private:
  forward_iterator (const KeyT &aKey, Map const *const aMap, std::size_t aBucketIndex, int aValueIndex)
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
  std::size_t bucketIndex;
  int valueIndex;
  UniqueSharedLock valueLock;

  static std::unordered_map<const Map *, std::size_t> instances;
  static std::unordered_map<const Map *, std::shared_lock<std::shared_mutex>> locks;
  static std::mutex instancesMutex;

  friend Map;
  friend Bucket;
  friend InternalValue;
};

template <class KeyT, class ValueT, class HashFuncT>
std::unordered_map<const concurrent_unordered_map<KeyT, ValueT, HashFuncT> *, std::size_t>
  forward_iterator<KeyT, ValueT, HashFuncT>::instances;

template <class KeyT, class ValueT, class HashFuncT>
std::unordered_map<const concurrent_unordered_map<KeyT, ValueT, HashFuncT> *, std::shared_lock<std::shared_mutex>>
  forward_iterator<KeyT, ValueT, HashFuncT>::locks;

template <class KeyT, class ValueT, class HashFuncT>
std::mutex forward_iterator<KeyT, ValueT, HashFuncT>::instancesMutex;

#endif