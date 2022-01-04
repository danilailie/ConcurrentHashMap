#ifndef _CONCURRENT_HASH_MAP_HPP_
#define _CONCURRENT_HASH_MAP_HPP_

#include <atomic>
#include <chrono>
#include <functional>
#include <shared_mutex>
#include <vector>

#include "bucket.hpp"
#include "forward_iterator.hpp"
#include "internal_value.hpp"

template <class KeyT, class ValueT, class HashFuncT = std::hash<KeyT>> class concurrent_unordered_map
{
public:
  using iterator = forward_iterator<KeyT, ValueT, HashFuncT>;
  using const_iterator = const forward_iterator<KeyT, ValueT, HashFuncT>;
  using UniqueSharedLock = std::unique_ptr<std::shared_lock<std::shared_mutex>>;

public:
  /// <summary>Constructor</summary>
  /// <param name="bucketCount">How many buckets to start with</param>
  /// <returns></returns>
  concurrent_unordered_map (std::size_t bucketCount = 31);

  /// <summary>Gets the number of elements in the map</summary>
  /// <param></param>
  /// <returns></returns>
  std::size_t getSize () const;

  /// <summary></summary>
  /// <param></param>
  /// <returns>Begin iterator</returns>
  iterator begin () const;
  const_iterator
  cbegin () const
  {
    return begin ();
  };

  /// <summary></summary>
  /// <param></param>
  /// <returns>End iterator</returns>
  iterator end () const;
  const_iterator
  cend () const
  {
    return end ();
  }

  /// <summary>Inserts a key-value pair into the map</summary>
  /// <param name="aKeyValuePair">The pair to be inserted</param>
  /// <returns>A pair containing an iterator (can be end) and a bool result, true if operation has succeded.</returns>
  std::pair<iterator, bool> insert (std::pair<const KeyT &, const ValueT &> aKeyValuePair);

  /// <summary>Inserts a key and a value into the map</summary>
  /// <param name="aKey">The key</param>
  /// <param name="aValue">The value</param>
  /// <returns>A pair containing an iterator (can be end) and a bool result, true if operation has succeded.</returns>
  std::pair<iterator, bool> insert (const KeyT &aKey, const ValueT &aValue);

  /// <summary>Finds an element with a key in the map.</summary>
  /// <param name="aKey">The key</param>
  /// <returns>Iterator to the found element (will be end() if key is not found).</returns>
  iterator find (const KeyT &aKey) const;

  /// <summary>Erases the element pointed by the iterator. Invalidates the iterator</summary>
  /// <param name="anIterator">The iterator</param>
  /// <returns>True if element was present in the map (IE iterator was valid).</returns>
  bool erase (const iterator &anIterator);

  /// <summary>Erases the element with the key param. Invalidates any iterator to this element.</summary>
  /// <param name="aKey">The key</param>
  /// <returns>True if element was present in the map.</returns>
  bool erase (const KeyT &aKey);

  /// <summary>Increases the number of buckets and starts moving all valid (not erased) to the new buckets.</summary>
  /// <param ></param>
  /// <returns></returns>
  void rehash ();

private:
  using InternalValue = internal_value<KeyT, ValueT, HashFuncT>;
  using BucketType = bucket<KeyT, ValueT, HashFuncT>;

private:
  std::pair<KeyT, ValueT> &getIterValue (const KeyT &aKey) const;
  std::pair<KeyT, ValueT> &getIterValue (const iterator &anIter) const;
  std::pair<KeyT, ValueT> *getIterPtr (const iterator &anIter) const;
  std::size_t getNextPopulatedBucketIndex (std::size_t anIndex) const;

  /// <summary>Gets the key of the first element - equivalent to begin()</summary>
  /// <param></param>
  /// <returns></returns>
  KeyT getFirstKey () const;

  /// <summary>Gets the key of the valid element that comes after the reference one. If there is no such element returns
  /// an invalid key.</summary> <param name="bucketIndex"></param> <param name="valueIndex"></param> <returns></returns>
  KeyT getNextElement (std::size_t &bucketIndex, int &valueIndex) const;

  void advanceIterator (iterator &it) const;

  void lockResource (std::size_t &bucketIndex, int &valueIndex) const;

  void unlockResource (std::size_t &bucketIndex, int &valueIndex) const;

  /// <summary>Physically delete the previously removed(erased) elements.</summary>
  /// <param></param>
  /// <returns></returns>
  void eraseUnavailableValues ();

  std::size_t
  getNextPrimeNumber (const std::size_t &aValue)
  {
    std::vector<std::size_t> primes = { 43, 47, 53, 59, 61, 67, 71, 73, 79, 83, 89, 97, 10007, 20021, 40063 };
    for (std::size_t i = 0; i < primes.size (); ++i)
      {
	if (primes[i] > aValue)
	  {
	    return primes[i];
	  }
      }
    return 10007;
  }

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
concurrent_unordered_map<KeyT, ValueT, HashFuncT>::concurrent_unordered_map (std::size_t bucketCount)
{
  buckets.resize (bucketCount);
  currentBucketCount = bucketCount;
}

template <class KeyT, class ValueT, class HashFuncT>
std::size_t
concurrent_unordered_map<KeyT, ValueT, HashFuncT>::getSize () const
{
  return valueCount - erasedCount;
}

template <class KeyT, class ValueT, class HashFuncT>
typename concurrent_unordered_map<KeyT, ValueT, HashFuncT>::iterator
concurrent_unordered_map<KeyT, ValueT, HashFuncT>::begin () const
{
  for (int i = 0; i < int (buckets.size ()); ++i)
    {
      if (buckets[i].getSize () > 0)
	{
	  return buckets[i].begin (this, i);
	  //   int valueIndex = -1;
	  //   auto key = buckets[i].getFirstKey (valueIndex);
	  //   return iterator (key, this, i, valueIndex);
	}
    }
  return end ();
}

template <class KeyT, class ValueT, class HashFuncT>
typename concurrent_unordered_map<KeyT, ValueT, HashFuncT>::iterator
concurrent_unordered_map<KeyT, ValueT, HashFuncT>::end () const
{
  return iterator (InvalidKeyValue<KeyT> (), this, -1, -1);
}

template <class KeyT, class ValueT, class HashFuncT>
std::pair<typename concurrent_unordered_map<KeyT, ValueT, HashFuncT>::iterator, bool>
concurrent_unordered_map<KeyT, ValueT, HashFuncT>::insert (std::pair<const KeyT &, const ValueT &> aKeyValuePair)
{
  return insert (aKeyValuePair.first, aKeyValuePair.second);
}

template <class KeyT, class ValueT, class HashFuncT>
std::pair<typename concurrent_unordered_map<KeyT, ValueT, HashFuncT>::iterator, bool>
concurrent_unordered_map<KeyT, ValueT, HashFuncT>::insert (const KeyT &aKey, const ValueT &aValue)
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

  auto it = added ? buckets[bucketIndex].getIterator (this, bucketIndex, position) : end ();

  return std::make_pair (it, added);
}

template <class KeyT, class ValueT, class HashFuncT>
typename concurrent_unordered_map<KeyT, ValueT, HashFuncT>::iterator
concurrent_unordered_map<KeyT, ValueT, HashFuncT>::find (const KeyT &aKey) const
{
  auto hashResult = hashFunc (aKey);
  auto bucketIndex = hashResult % currentBucketCount;

  auto position = buckets[bucketIndex].find (aKey);
  if (position != -1)
    {
      return buckets[bucketIndex].getIterator (this, bucketIndex, position);
    }
  return end ();
}

template <class KeyT, class ValueT, class HashFuncT>
bool
concurrent_unordered_map<KeyT, ValueT, HashFuncT>::erase (const iterator &anIterator)
{
  return erase (anIterator.getKey ());
}

template <class KeyT, class ValueT, class HashFuncT>
bool
concurrent_unordered_map<KeyT, ValueT, HashFuncT>::erase (const KeyT &aKey)
{
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
      return true;
    }
  return false;
}

template <class KeyT, class ValueT, class HashFuncT>
std::pair<KeyT, ValueT> &
concurrent_unordered_map<KeyT, ValueT, HashFuncT>::getIterValue (const KeyT &aKey) const
{
  auto hashResult = hashFunc (aKey);
  auto bucketIndex = hashResult % currentBucketCount;

  return buckets[bucketIndex].getKeyValuePair (aKey);
}

template <class KeyT, class ValueT, class HashFuncT>
std::pair<KeyT, ValueT> &
concurrent_unordered_map<KeyT, ValueT, HashFuncT>::getIterValue (const iterator &anIter) const
{
  const std::pair<KeyT, ValueT> *keyValue = &(buckets[anIter.bucketIndex].values[anIter.valueIndex].keyValue);
  return const_cast<std::pair<KeyT, ValueT> &> (*keyValue);
}

template <class KeyT, class ValueT, class HashFuncT>
std::pair<KeyT, ValueT> *
concurrent_unordered_map<KeyT, ValueT, HashFuncT>::getIterPtr (const iterator &anIter) const
{
  const std::pair<KeyT, ValueT> *keyValue = &(buckets[anIter.bucketIndex].values[anIter.valueIndex].keyValue);
  return const_cast<std::pair<KeyT, ValueT> *> (keyValue);
}

template <class KeyT, class ValueT, class HashFuncT>
std::size_t
concurrent_unordered_map<KeyT, ValueT, HashFuncT>::getNextPopulatedBucketIndex (std::size_t anIndex) const
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
concurrent_unordered_map<KeyT, ValueT, HashFuncT>::getFirstKey () const
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
concurrent_unordered_map<KeyT, ValueT, HashFuncT>::getNextElement (std::size_t &bucketIndex, int &valueIndex) const
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
	  int position;
	  return buckets[nextBucketIndex].getFirstKey (position);
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
concurrent_unordered_map<KeyT, ValueT, HashFuncT>::advanceIterator (iterator &it) const
{
  std::size_t nextBucketIndex = it.bucketIndex;

  bool found = false;
  do
    {
      if (buckets[nextBucketIndex].advanceIterator (it, nextBucketIndex))
	{
	  found = true;
	}
      else
	{
	  ++nextBucketIndex;
	}
    }
  while (!found && nextBucketIndex < buckets.size ());

  if (!found)
    {
      it = end ();
    }
}

template <class KeyT, class ValueT, class HashFuncT>
void
concurrent_unordered_map<KeyT, ValueT, HashFuncT>::lockResource (std::size_t &bucketIndex, int &valueIndex) const
{
  buckets[bucketIndex].values[valueIndex].lock ();
}

template <class KeyT, class ValueT, class HashFuncT>
void
concurrent_unordered_map<KeyT, ValueT, HashFuncT>::unlockResource (std::size_t &bucketIndex, int &valueIndex) const
{
  buckets[bucketIndex].values[valueIndex].unlock ();
}

template <class KeyT, class ValueT, class HashFuncT>
void
concurrent_unordered_map<KeyT, ValueT, HashFuncT>::eraseUnavailableValues ()
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
concurrent_unordered_map<KeyT, ValueT, HashFuncT>::rehash ()
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
