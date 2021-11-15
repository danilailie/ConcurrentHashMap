#ifndef _FORWARD_ITERATOR_HPP_
#define _FORWARD_ITERATOR_HPP_

#include "RandomAccessIterator.hpp"

template <class KeyT, class ValueT, class HashFuncT> class ConcurrentHashMap;

template <class KeyT, class ValueT, class HashFuncT> class ForwardIteratorType
{
public:
  using RAIterator = RandomAccessIteratorType<KeyT, ValueT, HashFuncT>;
  using Map = ConcurrentHashMap<KeyT, ValueT, HashFuncT>;

  ForwardIteratorType (const ForwardIteratorType &other)
  {
    map = other.map;
    bucketIndex = other.bucketIndex;
    valueIndex = other.valueIndex;
    instances[map]++;
    if (instances[aMap] == 1)
      {
	locks.insert (std::make_pair (aMap, std::shared_lock<std::shared_mutex> (aMap->rehashMutex)));
      }
  }

  ~ForwardIteratorType ()
  {
    instances[map]--;

    if (instances[map] == 0)
      {
	instances.erase (map);
	locks.erase (map);
      }
  }

  ForwardIteratorType &
  operator= (const ForwardIteratorType &other)
  {
    if (this == &other)
      {
	return this;
      }

    map = other.map;
    bucketIndex = other.bucketIndex;
    valueIndex = other.valueIndex;

    return *this;
  }

  std::pair<KeyT, ValueT>
  operator* ()
  {
    return map->getIterValue (*this);
  }

  bool
  operator== (const ForwardIteratorType &another)
  {
    return map == another.map && key == another.key;
  }

  bool
  operator== (const RAIterator &another)
  {
    return map == another.map && key == another.key;
  }

  bool
  operator!= (const ForwardIteratorType &other)
  {
    return map != other.map || key != other.key;
  }

  bool
  operator!= (const RAIterator &other)
  {
    return map != other.map || key != other.key;
  }

  ForwardIteratorType &
  operator++ ()
  {
    key = map->getNextElement (bucketIndex, valueIndex);
    return *this;
  }

  ForwardIteratorType &
  operator++ (int)
  {
    ForwardIteratorType tmp = *this;
    ++(*this);
    return tmp;
  }

private:
  ForwardIteratorType (const KeyT &aKey, Map *aMap, std::size_t aBucketIndex, std::size_t aValueIndex)
      : key (aKey), map (aMap), bucketIndex (aBucketIndex), valueIndex (aValueIndex)
  {
    instances[aMap]++;

    if (instances[aMap] == 1)
      {
	locks.insert (std::make_pair (aMap, std::shared_lock<std::shared_mutex> (aMap->rehashMutex)));
      }
  }

private:
  Map *map;
  KeyT key; // used for compare between iterator types.
  std::size_t bucketIndex;
  std::size_t valueIndex;

  static std::unordered_map<Map *, std::size_t> instances;
  static std::unordered_map<Map *, std::shared_lock<std::shared_mutex>> locks;

  friend Map;
};

template <class KeyT, class ValueT, class HashFuncT>
std::unordered_map<ConcurrentHashMap<KeyT, ValueT, HashFuncT> *, std::size_t>
  ForwardIteratorType<KeyT, ValueT, HashFuncT>::instances;

template <class KeyT, class ValueT, class HashFuncT>
std::unordered_map<ConcurrentHashMap<KeyT, ValueT, HashFuncT> *, std::shared_lock<std::shared_mutex>>
  ForwardIteratorType<KeyT, ValueT, HashFuncT>::locks;

#endif