﻿#ifndef _CONCURRENT_HASH_MAP_HPP_
#define _CONCURRENT_HASH_MAP_HPP_

#include <atomic>
#include <chrono>
#include <functional>
#include <shared_mutex>
#include <vector>

#include "Bucket.hpp"
#include "ForwardIterator.hpp"
#include "HashMapUtils.hpp"
#include "InternalValue.hpp"

template <class KeyT, class ValueT, class HashFuncT = std::hash<KeyT>> class ConcurrentHashMap
{
public:
  using iterator = ForwardIteratorType<KeyT, ValueT, HashFuncT>;
  using const_iterator = const ForwardIteratorType<KeyT, ValueT, HashFuncT>;

public:
  ConcurrentHashMap (std::size_t bucketCount = 31);

  std::size_t getSize () const;

  iterator begin () const;
  const_iterator
  cbegin () const
  {
    return begin ();
  };

  iterator end () const;
  const_iterator
  cend () const
  {
    return end ();
  }

  std::pair<iterator, bool> insert (std::pair<const KeyT &, const ValueT &> aKeyValuePair);
  std::pair<iterator, bool> insert (const KeyT &aKey, const ValueT &aValue);

  iterator find (const KeyT &aKey) const;

  iterator erase (const iterator &anIterator);

  iterator erase (const KeyT &aKey);

  void rehash ();

private:
  using InternalValue = InternalValueType<KeyT, ValueT, HashFuncT>;
  using BucketType = Bucket<KeyT, ValueT, HashFuncT>;

private:
  std::pair<KeyT, ValueT> &getIterValue (const KeyT &aKey) const;
  std::pair<KeyT, ValueT> &getIterValue (const iterator &anIter) const;
  std::pair<KeyT, ValueT> *getIterPtr (const iterator &anIter) const;
  std::size_t getNextPopulatedBucketIndex (std::size_t anIndex) const;
  KeyT getFirstKey () const;
  KeyT getNextElement (std::size_t &bucketIndex, int &valueIndex) const;
  void eraseUnavailableValues ();

private:
  HashFuncT hashFunc;
  std::vector<BucketType> buckets;
  mutable std::shared_mutex rehashMutex;
  std::size_t currentBucketCount;
  std::atomic<std::size_t> valueCount;
  std::atomic<std::size_t> erasedCount;

  friend iterator;
};

template <class KeyT, class ValueT, class HashFuncT>
ConcurrentHashMap<KeyT, ValueT, HashFuncT>::ConcurrentHashMap (std::size_t bucketCount)
{
  buckets.resize (bucketCount);
  currentBucketCount = bucketCount;
}

template <class KeyT, class ValueT, class HashFuncT>
std::size_t
ConcurrentHashMap<KeyT, ValueT, HashFuncT>::getSize () const
{
  return valueCount - erasedCount;
}

template <class KeyT, class ValueT, class HashFuncT>
typename ConcurrentHashMap<KeyT, ValueT, HashFuncT>::iterator
ConcurrentHashMap<KeyT, ValueT, HashFuncT>::begin () const
{
  for (std::size_t i = 0; i < buckets.size (); ++i)
    {
      if (buckets[i].getSize () > 0)
	{
	  auto valueIndex = buckets[i].getFirstValueIndex ();
	  auto key = buckets[i].getKeyAt (valueIndex);
	  return iterator (key, this, i, valueIndex);
	}
    }
  return iterator (InvalidKeyValue<KeyT> (), this, -1, -1);
}

template <class KeyT, class ValueT, class HashFuncT>
typename ConcurrentHashMap<KeyT, ValueT, HashFuncT>::iterator
ConcurrentHashMap<KeyT, ValueT, HashFuncT>::end () const
{
  return iterator (InvalidKeyValue<KeyT> (), this, -1, -1);
}

template <class KeyT, class ValueT, class HashFuncT>
std::pair<typename ConcurrentHashMap<KeyT, ValueT, HashFuncT>::iterator, bool>
ConcurrentHashMap<KeyT, ValueT, HashFuncT>::insert (std::pair<const KeyT &, const ValueT &> aKeyValuePair)
{
  return insert (aKeyValuePair.first, aKeyValuePair.second);
}

template <class KeyT, class ValueT, class HashFuncT>
std::pair<typename ConcurrentHashMap<KeyT, ValueT, HashFuncT>::iterator, bool>
ConcurrentHashMap<KeyT, ValueT, HashFuncT>::insert (const KeyT &aKey, const ValueT &aValue)
{
  auto hashResult = hashFunc (aKey);
  auto bucketIndex = hashResult % currentBucketCount;

  bool added = false;
  int position = buckets[bucketIndex].insert (aKey, aValue);
  if (position != -1)
    {
      valueCount++;
      added = true;
    }

  return std::make_pair (ConcurrentHashMap<KeyT, ValueT, HashFuncT>::iterator (aKey, this, bucketIndex, position),
			 added);
}

template <class KeyT, class ValueT, class HashFuncT>
typename ConcurrentHashMap<KeyT, ValueT, HashFuncT>::iterator
ConcurrentHashMap<KeyT, ValueT, HashFuncT>::find (const KeyT &aKey) const
{
  auto hashResult = hashFunc (aKey);
  auto bucketIndex = hashResult % currentBucketCount;

  auto position = buckets[bucketIndex].find (aKey);
  if (position != -1)
    {
      return iterator (aKey, this, bucketIndex, position);
    }
  return end ();
}

template <class KeyT, class ValueT, class HashFuncT>
typename ConcurrentHashMap<KeyT, ValueT, HashFuncT>::iterator
ConcurrentHashMap<KeyT, ValueT, HashFuncT>::erase (const iterator &anIterator)
{
  return erase (anIterator.getKey ());
}

template <class KeyT, class ValueT, class HashFuncT>
typename ConcurrentHashMap<KeyT, ValueT, HashFuncT>::iterator
ConcurrentHashMap<KeyT, ValueT, HashFuncT>::erase (const KeyT &aKey)
{
  // TODO: Ilie check this.
  // std::unique_lock<std::shared_mutex> lock (rehashMutex);

  auto hashResult = hashFunc (aKey);
  auto bucketIndex = hashResult % currentBucketCount;

  int position = buckets[bucketIndex].erase (aKey);

  if (position != -1)
    {
      erasedCount++;
    }

  if (erasedCount >= valueCount / 2)
    {
      eraseUnavailableValues ();
    }

  if (position != -1)
    {
      return iterator (aKey, this, bucketIndex, position);
    }
  return end ();
}

template <class KeyT, class ValueT, class HashFuncT>
std::pair<KeyT, ValueT> &
ConcurrentHashMap<KeyT, ValueT, HashFuncT>::getIterValue (const KeyT &aKey) const
{
  auto hashResult = hashFunc (aKey);
  auto bucketIndex = hashResult % currentBucketCount;

  return buckets[bucketIndex].getKeyValuePair (aKey);
}

template <class KeyT, class ValueT, class HashFuncT>
std::pair<KeyT, ValueT> &
ConcurrentHashMap<KeyT, ValueT, HashFuncT>::getIterValue (const iterator &anIter) const
{
  const std::pair<KeyT, ValueT> *keyValue = &(buckets[anIter.bucketIndex].values[anIter.valueIndex].keyValue);
  return const_cast<std::pair<KeyT, ValueT> &> (*keyValue);
}

template <class KeyT, class ValueT, class HashFuncT>
std::pair<KeyT, ValueT> *
ConcurrentHashMap<KeyT, ValueT, HashFuncT>::getIterPtr (const iterator &anIter) const
{
  const std::pair<KeyT, ValueT> *keyValue = &(buckets[anIter.bucketIndex].values[anIter.valueIndex].keyValue);
  return const_cast<std::pair<KeyT, ValueT> *> (keyValue);
}

template <class KeyT, class ValueT, class HashFuncT>
std::size_t
ConcurrentHashMap<KeyT, ValueT, HashFuncT>::getNextPopulatedBucketIndex (std::size_t anIndex) const
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
ConcurrentHashMap<KeyT, ValueT, HashFuncT>::getFirstKey () const
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
ConcurrentHashMap<KeyT, ValueT, HashFuncT>::getNextElement (std::size_t &bucketIndex, int &valueIndex) const
{
  int nextValueIndex = buckets[bucketIndex].getNextValueIndex (valueIndex);

  if (nextValueIndex == -1)
    {
      int nextBucketIndex = int (getNextPopulatedBucketIndex (bucketIndex));
      if (nextBucketIndex == -1)
	{
	  return InvalidKeyValue<KeyT> ();
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
ConcurrentHashMap<KeyT, ValueT, HashFuncT>::eraseUnavailableValues ()
{
  if (rehashMutex.try_lock ())
    {
      valueCount = 0;
      for (std::size_t i = 0; i < buckets.size (); ++i)
	{
	  valueCount += buckets[i].eraseUnavailableValues ();
	}
      erasedCount = 0;
      rehashMutex.unlock ();
    }
}

template <class KeyT, class ValueT, class HashFuncT>
void
ConcurrentHashMap<KeyT, ValueT, HashFuncT>::rehash ()
{
  std::unique_lock<std::shared_mutex> lock (rehashMutex);

  currentBucketCount = getNextPrimeNumber (currentBucketCount * 2);
  std::vector<BucketType> newBuckets;
  newBuckets.resize (currentBucketCount);

  for (auto i = 0; i < buckets.size (); ++i)
    {
      std::unique_lock<std::shared_mutex> bucketLock (*(buckets[i].bucketMutex));
      for (auto j = 0; j < buckets[i].values.size (); ++j)
	{
	  if (buckets[i].values[j].isAvailable ())
	    {
	      auto hashResult = hashFunc (buckets[i].values[j].keyValue.first);
	      auto bucketIndex = hashResult % currentBucketCount;
	      newBuckets[bucketIndex].insert (buckets[i].values[j].getKeyValuePair ());
	    }
	}
    }

  buckets = std::move (newBuckets);
}

#endif
