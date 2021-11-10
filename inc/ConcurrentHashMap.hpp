#ifndef _CONCURRENT_HASH_MAP_HPP_
#define _CONCURRENT_HASH_MAP_HPP_

#include <atomic>
#include <functional>
#include <shared_mutex>
#include <vector>

#include "Bucket.hpp"
#include "ForwardIterator.hpp"
#include "InternalValue.hpp"
#include "RandomAccessIterator.hpp"

std::size_t
getNextPrimeNumber (const std::size_t &aValue)
{
  std::vector<std::size_t> primes = { 2,  3,  5,  7,  11, 13, 17, 19, 23, 29, 31, 37, 41,
				      43, 47, 53, 59, 61, 67, 71, 73, 79, 83, 89, 97 };
  for (auto i = 0; i < primes.size (); ++i)
    {
      if (primes[i] > aValue)
	{
	  return primes[i];
	}
    }
  return 10007;
}
template <class KeyT, class ValueT, class HashFuncT = std::hash<KeyT>> class ConcurrentHashMap
{
public:
  using RAIterator = RandomAccessIteratorType<KeyT, ValueT, HashFuncT>;
  using FWIterator = ForwardIteratorType<KeyT, ValueT, HashFuncT>;

public:
  ConcurrentHashMap ();

  std::size_t getSize () const;

  FWIterator begin ();

  RAIterator end ();

  RAIterator insert (const KeyT &aKey, const ValueT &aValue);

  RAIterator find (const KeyT &aKey);

  RAIterator erase (const RAIterator &anIterator);

  RAIterator erase (const KeyT &aKey);

  void rehash ();

private:
  using InternalValue = InternalValueType<KeyT, ValueT, HashFuncT>;
  using Bucket = Bucket<KeyT, ValueT, HashFuncT>;

private:
  std::pair<KeyT, ValueT> getIterValue (const KeyT &aKey);
  std::pair<KeyT, ValueT> getIterValue (const FWIterator &anIter);
  std::size_t getNextPopulatedBucketIndex (std::size_t anIndex);
  KeyT getFirstKey ();
  KeyT getNextElement (std::size_t &bucketIndex, std::size_t &valueIndex);

private:
  HashFuncT hashFunc;
  std::vector<Bucket> buckets;
  std::mutex rehashMutex;
  std::size_t currentMaxSize = 3; // prime

  friend RAIterator;
  friend FWIterator;
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
typename ConcurrentHashMap<KeyT, ValueT, HashFuncT>::FWIterator
ConcurrentHashMap<KeyT, ValueT, HashFuncT>::begin ()
{
  for (auto i = 0; i < buckets.size (); ++i)
    {
      if (buckets[i].getSize () > 0)
	{
	  auto valueIndex = buckets[i].getFirstValueIndex ();
	  auto key = buckets[i].getKeyAt (valueIndex);
	  return FWIterator (key, this, i, valueIndex);
	}
    }
  return FWIterator ();
}

template <class KeyT, class ValueT, class HashFuncT>
typename ConcurrentHashMap<KeyT, ValueT, HashFuncT>::RAIterator
ConcurrentHashMap<KeyT, ValueT, HashFuncT>::end ()
{
  return RAIterator (-1, this);
}

template <class KeyT, class ValueT, class HashFuncT>
typename ConcurrentHashMap<KeyT, ValueT, HashFuncT>::RAIterator
ConcurrentHashMap<KeyT, ValueT, HashFuncT>::insert (const KeyT &aKey, const ValueT &aValue)
{
  auto hashResult = hashFunc (aKey);
  auto bucketIndex = hashResult % currentMaxSize;

  buckets[bucketIndex].insert (aKey, aValue);

  return ConcurrentHashMap<KeyT, ValueT, HashFuncT>::RAIterator (aKey, this);
}

template <class KeyT, class ValueT, class HashFuncT>
typename ConcurrentHashMap<KeyT, ValueT, HashFuncT>::RAIterator
ConcurrentHashMap<KeyT, ValueT, HashFuncT>::find (const KeyT &aKey)
{
  auto hashResult = hashFunc (aKey);
  auto bucketIndex = hashResult % currentMaxSize;

  return RAIterator (buckets[bucketIndex].find (aKey), this);
}

template <class KeyT, class ValueT, class HashFuncT>
typename ConcurrentHashMap<KeyT, ValueT, HashFuncT>::RAIterator
ConcurrentHashMap<KeyT, ValueT, HashFuncT>::erase (const RAIterator &anIterator)
{
  return erase (anIterator.getKey ());
}

template <class KeyT, class ValueT, class HashFuncT>
typename ConcurrentHashMap<KeyT, ValueT, HashFuncT>::RAIterator
ConcurrentHashMap<KeyT, ValueT, HashFuncT>::erase (const KeyT &aKey)
{
  auto hashResult = hashFunc (aKey);
  auto bucketIndex = hashResult % currentMaxSize;
  return RAIterator (buckets[bucketIndex].erase (aKey), this);
}

template <class KeyT, class ValueT, class HashFuncT>
std::pair<KeyT, ValueT>
ConcurrentHashMap<KeyT, ValueT, HashFuncT>::getIterValue (const KeyT &aKey)
{
  auto hashResult = hashFunc (aKey);
  auto bucketIndex = hashResult % currentMaxSize;

  return buckets[bucketIndex].getKeyValuePair (aKey);
}

template <class KeyT, class ValueT, class HashFuncT>
std::pair<KeyT, ValueT>
ConcurrentHashMap<KeyT, ValueT, HashFuncT>::getIterValue (const FWIterator &anIter)
{
  return std::make_pair (buckets[anIter.bucketIndex].values[anIter.valueIndex].key,
			 buckets[anIter.bucketIndex].values[anIter.valueIndex].userValue);
}

template <class KeyT, class ValueT, class HashFuncT>
std::size_t
ConcurrentHashMap<KeyT, ValueT, HashFuncT>::getNextPopulatedBucketIndex (std::size_t anIndex)
{
  for (auto i = anIndex + 1; i < buckets.size (); ++i)
    {
      if (buckets[i].getSize () > 0)
	{
	  return i;
	}
    }
  return -1;
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
KeyT
ConcurrentHashMap<KeyT, ValueT, HashFuncT>::getNextElement (std::size_t &bucketIndex, std::size_t &valueIndex)
{
  auto nextValueIndex = buckets[bucketIndex].getNextValueIndex (valueIndex);

  if (nextValueIndex == -1)
    {
      auto nextBucketIndex = getNextPopulatedBucketIndex (bucketIndex);
      if (nextBucketIndex == -1)
	{
	  return -1;
	}
      else
	{
	  bucketIndex = nextBucketIndex;
	  valueIndex = buckets[nextBucketIndex].getFirstValueIndex ();
	  return buckets[nextBucketIndex].getKeyAt (valueIndex);
	}
    }
  else
    {
      valueIndex = nextValueIndex;
      return buckets[bucketIndex].getKeyAt (valueIndex);
    }
}

template <class KeyT, class ValueT, class HashFuncT>
void
ConcurrentHashMap<KeyT, ValueT, HashFuncT>::rehash ()
{
  currentMaxSize = getNextPrimeNumber (currentMaxSize * 2);

  std::vector<Bucket> newBuckets;
  newBuckets.resize (currentMaxSize);

  std::unique_lock<std::mutex> lock (rehashMutex);

  for (auto i = 0; i < buckets.size (); ++i)
    {
      std::unique_lock<std::shared_mutex> bucketLock (*(buckets[i].bucketMutex));
      for (auto j = 0; j < buckets[i].values.size (); ++j)
	{
	  if (buckets[i].values[j].isAvailable ())
	    {
	      auto hashResult = hashFunc (buckets[i].values[j].key);
	      auto bucketIndex = hashResult % currentMaxSize;
	      newBuckets[bucketIndex].insert (buckets[i].values[j].getKeyValuePair ());
	    }
	}
    }

  buckets = std::move (newBuckets);
}

#endif
