#ifndef _CONCURRENT_HASH_MAP_HPP_
#define _CONCURRENT_HASH_MAP_HPP_

#include <atomic>
#include <functional>
#include <shared_mutex>
#include <vector>

#include "Bucket.hpp"
#include "InternalValue.hpp"
#include "RandomAccessIterator.hpp"

std::size_t
getNextPrimeNumber (const std::size_t &aValue)
{
  std::vector<std::size_t> primes = { 37, 41, 43, 47, 53, 59, 61, 67, 71, 73, 79, 83, 89, 97 };
  for (auto i = 0; i < primes.size (); ++i)
    {
      if (primes[i] > aValue * 2)
	{
	  return primes[i];
	}
    }
  return 10007;
}
template <class KeyT, class ValueT, class HashFuncT = std::hash<KeyT>> class ConcurrentHashMap
{
public:
  using Iterator = RandomAccessIteratorType<KeyT, ValueT, HashFuncT>;

public:
  ConcurrentHashMap ();

  std::size_t getSize () const;

  Iterator begin ();

  Iterator end ();

  Iterator insert (const KeyT &aKey, const ValueT &aValue);

  Iterator find (const KeyT &aKey);

  Iterator erase (const Iterator anIterator);

  Iterator erase (const KeyT &aKey);

  void rehash ();

private:
  using InternalValue = InternalValueType<KeyT, ValueT, HashFuncT>;
  using Bucket = Bucket<KeyT, ValueT, HashFuncT>;

private:
  std::pair<KeyT, ValueT> getIterValue (const KeyT &aKey);
  KeyT getFirstKey ();

private:
  HashFuncT hashFunc;
  std::vector<Bucket> buckets;
  std::mutex rehashMutex;
  std::size_t currentMaxSize = 31; // prime

  friend Iterator;
};

template <class KeyT, class ValueT, class HashFuncT> ConcurrentHashMap<KeyT, ValueT, HashFuncT>::ConcurrentHashMap ()
{
  buckets.resize (currentMaxSize);
}

template <class KeyT, class ValueT, class HashFuncT>
std::size_t
ConcurrentHashMap<KeyT, ValueT, HashFuncT>::getSize () const
{
  return buckets.size ();
}

template <class KeyT, class ValueT, class HashFuncT>
typename ConcurrentHashMap<KeyT, ValueT, HashFuncT>::Iterator
ConcurrentHashMap<KeyT, ValueT, HashFuncT>::begin ()
{
  return Iterator (getFirstKey (), this);
}

template <class KeyT, class ValueT, class HashFuncT>
typename ConcurrentHashMap<KeyT, ValueT, HashFuncT>::Iterator
ConcurrentHashMap<KeyT, ValueT, HashFuncT>::end ()
{
  return Iterator (-1, this);
}

template <class KeyT, class ValueT, class HashFuncT>
typename ConcurrentHashMap<KeyT, ValueT, HashFuncT>::Iterator
ConcurrentHashMap<KeyT, ValueT, HashFuncT>::insert (const KeyT &aKey, const ValueT &aValue)
{
  auto hashResult = hashFunc (aKey);
  auto bucketIndex = hashResult % currentMaxSize;

  std::unique_lock<std::shared_mutex> bucketLock (*buckets[bucketIndex].bucketMutex);
  buckets[bucketIndex].values.push_back (InternalValue (aKey, aValue));

  return ConcurrentHashMap<KeyT, ValueT, HashFuncT>::Iterator (aKey, this);
}

template <class KeyT, class ValueT, class HashFuncT>
typename ConcurrentHashMap<KeyT, ValueT, HashFuncT>::Iterator
ConcurrentHashMap<KeyT, ValueT, HashFuncT>::find (const KeyT &aKey)
{
  auto hashResult = hashFunc (aKey);
  auto bucketIndex = hashResult % currentMaxSize;
  std::shared_lock<std::shared_mutex> bucketLock (*buckets[bucketIndex].bucketMutex);
  for (auto i = 0; i < buckets[bucketIndex].values.size (); ++i)
    {
      if (buckets[bucketIndex].values[i].compareKey (aKey))
	{
	  return Iterator (aKey, this);
	}
    }
  return end ();
}

template <class KeyT, class ValueT, class HashFuncT>
typename ConcurrentHashMap<KeyT, ValueT, HashFuncT>::Iterator
ConcurrentHashMap<KeyT, ValueT, HashFuncT>::erase (const Iterator anIterator)
{
  return erase (anIterator.getKey ());
}

template <class KeyT, class ValueT, class HashFuncT>
typename ConcurrentHashMap<KeyT, ValueT, HashFuncT>::Iterator
ConcurrentHashMap<KeyT, ValueT, HashFuncT>::erase (const KeyT &aKey)
{
  auto hashResult = hashFunc (aKey);
  auto bucketIndex = hashResult % currentMaxSize;
  std::shared_lock<std::shared_mutex> bucketLock (*buckets[bucketIndex].bucketMutex);
  for (auto i = 0; i < buckets[bucketIndex].values.size (); ++i)
    {
      if (buckets[bucketIndex].values[i].compareKey (aKey))
	{
	  buckets[bucketIndex].values[i].erase ();
	  return IteratorT (aKey, this);
	}
    }
  return end ();
}

template <class KeyT, class ValueT, class HashFuncT>
std::pair<KeyT, ValueT>
ConcurrentHashMap<KeyT, ValueT, HashFuncT>::getIterValue (const KeyT &aKey)
{
  auto hashResult = hashFunc (aKey);
  auto bucketIndex = hashResult % currentMaxSize;

  std::shared_lock<std::shared_mutex> bucketLock (*buckets[bucketIndex].bucketMutex);
  for (auto i = 0; i < buckets[bucketIndex].values.size (); ++i)
    {
      if (buckets[bucketIndex].values[i].compareKey (aKey))
	{
	  return buckets[bucketIndex].values[i].getKeyValuePair ();
	}
    }

  return std::pair<KeyT, ValueT> ();
}

template <class KeyT, class ValueT, class HashFuncT>
KeyT
ConcurrentHashMap<KeyT, ValueT, HashFuncT>::getFirstKey ()
{
  for (auto i = 0; i < buckets.size (); ++i)
    {
      if (buckets[i].getSize () > 0)
	{
	  return buckets[i].getFirstKey ();
	}
    }

  return -1;
}

template <class KeyT, class ValueT, class HashFuncT>
void
ConcurrentHashMap<KeyT, ValueT, HashFuncT>::rehash ()
{
  currentMaxSize = getNextPrimeNumber (currentMaxSize);

  std::vector<Bucket> newBuckets;
  newBuckets.resize (currentMaxSize);

  std::unique_lock<std::mutex> lock (rehashMutex);

  for (auto i = 0; i < buckets.size (); ++i)
    {
      std::unique_lock<std::shared_mutex> bucketLock (*(buckets[i].bucketMutex));
      for (auto j = 0; j < buckets[i].values.size (); ++j)
	{
	  std::unique_lock<std::shared_mutex> bucketLock (*(buckets[i].values[j].valueMutex));
	  if (!buckets[i].values[j].isMarkedForDelete)
	    {
	      auto hashResult = hashFunc (buckets[i].values[j].key);
	      auto bucketIndex = hashResult % currentMaxSize;
	      newBuckets[bucketIndex].values.push_back (
		InternalValue (buckets[i].values[j].key, buckets[i].values[j].userValue));
	    }
	}
    }

  buckets = std::move (newBuckets);
}

#endif
