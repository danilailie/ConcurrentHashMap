#ifndef _CONCURRENT_HASH_MAP_HPP_
#define _CONCURRENT_HASH_MAP_HPP_

#include <atomic>
#include <functional>
#include <shared_mutex>
#include <vector>

#include "Iterator.hpp"

template< class KeyT, class ValueT, class HashFuncT = std::hash< KeyT > >
class ConcurrentHashMap
{
public:
  using Iterator = Iterator<KeyT, ValueT, HashFuncT>;

private:
  struct InternalValue
  {
  private:
    std::unique_ptr< std::shared_mutex > valueMutex;
    bool isMarkedForDelete;
    KeyT key;
    ValueT userValue;

  public:
    InternalValue(const KeyT& aKey, const ValueT& aValue) : isMarkedForDelete(false), key(aKey), userValue(aValue)
    {
      valueMutex = std::make_unique< std::shared_mutex >();
    }

    bool compareKey(const KeyT& aKey)
    {
      std::shared_lock<std::shared_mutex> lock(*valueMutex);
      return key == aKey;
    }

    std::pair <KeyT, ValueT> getKeyValuePair()
    {
      return std::make_pair(key, userValue);
    }
  };

  struct Bucket
  {
    std::unique_ptr< std::shared_mutex > bucketMutex;
    std::vector< InternalValue > values;

    Bucket()
    {
      bucketMutex = std::make_unique< std::shared_mutex >();
    }
  };

public:
  ConcurrentHashMap();

  std::size_t getSize() const;

  Iterator begin();

  Iterator end();

  Iterator insert(const KeyT& aKey, const ValueT& aValue);

  Iterator find(const KeyT& aKey);

  Iterator erase(const Iterator anIterator);

private:
  std::pair<KeyT, ValueT> getIterValue(const KeyT& aKey);

private:
  HashFuncT hashFunc;
  std::vector< Bucket > buckets;

  std::size_t currentMaxSize = 32;

  friend class Iterator;
};

template< class KeyT, class ValueT, class HashFuncT>
ConcurrentHashMap< KeyT, ValueT, HashFuncT >::ConcurrentHashMap()
{
  buckets.resize(currentMaxSize);
}

template< class KeyT, class ValueT, class HashFuncT>
std::size_t ConcurrentHashMap< KeyT, ValueT, HashFuncT >::getSize() const
{
  return buckets.size();
}

template< class KeyT, class ValueT, class HashFuncT>
typename ConcurrentHashMap< KeyT, ValueT, HashFuncT >::Iterator ConcurrentHashMap< KeyT, ValueT, HashFuncT >::begin()
{
  return ConcurrentHashMap< KeyT, ValueT, HashFuncT >::Iterator(buckets[0].values[0].key);
}

template< class KeyT, class ValueT, class HashFuncT>
typename ConcurrentHashMap< KeyT, ValueT, HashFuncT >::Iterator ConcurrentHashMap< KeyT, ValueT, HashFuncT >::end()
{
  return ConcurrentHashMap< KeyT, ValueT, HashFuncT >::Iterator(-1);
}

template< class KeyT, class ValueT, class HashFuncT>
typename ConcurrentHashMap< KeyT, ValueT, HashFuncT >::Iterator ConcurrentHashMap< KeyT, ValueT, HashFuncT >::insert(
  const KeyT& aKey, const ValueT& aValue)
{
  auto hashResult = hashFunc(aKey);
  auto bucketIndex = hashResult % currentMaxSize;

  std::unique_lock<std::shared_mutex> bucketLock(*buckets[bucketIndex].bucketMutex);
  buckets[bucketIndex].values.push_back(InternalValue(aKey, aValue));

  return ConcurrentHashMap< KeyT, ValueT, HashFuncT >::Iterator(aKey, this);
}

template< class KeyT, class ValueT, class HashFuncT>
std::pair<KeyT, ValueT> ConcurrentHashMap< KeyT, ValueT, HashFuncT >::getIterValue(const KeyT& aKey)
{
  auto hashResult = hashFunc(aKey);
  auto bucketIndex = hashResult % currentMaxSize;

  std::shared_lock<std::shared_mutex> bucketLock(*buckets[bucketIndex].bucketMutex);
  for (auto i = 0; i < buckets[bucketIndex].values.size(); ++i)
  {
    if (buckets[bucketIndex].values[i].compareKey(aKey))
    {
      return buckets[bucketIndex].values[i].getKeyValuePair();
    }
  }

  return std::pair<KeyT, ValueT>();
}

#endif