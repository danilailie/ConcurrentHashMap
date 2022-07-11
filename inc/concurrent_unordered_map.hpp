#ifndef _CONCURRENT_HASH_MAP_HPP_
#define _CONCURRENT_HASH_MAP_HPP_

#include <atomic>
#include <cassert>
#include <chrono>
#include <functional>
#include <map>
#include <shared_mutex>
#include <vector>

#include "bucket.hpp"
#include "internal_value.hpp"
#include "iterator.hpp"
#include "performance_counters.hpp"
#include "unordered_map_utils.hpp"

template <class KeyT, class ValueT, class HashFuncT = std::hash<KeyT>> class concurrent_unordered_map
{
public:
  using iterator = Iterator<KeyT, ValueT, HashFuncT>;
  using const_iterator = const Iterator<KeyT, ValueT, HashFuncT>;

public:
  /// <summary>Constructor</summary>
  /// <param name="bucketCount">How many buckets to start with</param>
  /// <returns></returns>
  concurrent_unordered_map (std::size_t bucketCount = 500009, float erase_threshold_value = 0.7);

  /// <summary>Gets the number of elements in the map</summary>
  /// <param></param>
  /// <returns></returns>
  std::size_t size () const;

  /// <summary></summary>
  /// <param></param>
  /// <returns>Begin Iterator</returns>
  iterator begin () const;
  const_iterator
  cbegin () const
  {
    return begin ();
  };

  /// <summary></summary>
  /// <param></param>
  /// <returns>End Iterator</returns>
  iterator end () const;
  const_iterator
  cend () const
  {
    return end ();
  }

  /// <summary>Inserts a key-value pair into the map</summary>
  /// <param name="aKeyValuePair">The pair to be inserted</param>
  /// <returns>A pair containing an Iterator (can be end) and a bool result, true if operation has succeded.</returns>
  std::pair<iterator, bool> insert (const std::pair<KeyT, ValueT> &aKeyValuePair);

  /// <summary>Inserts a key and a value into the map</summary>
  /// <param name="aKey">The key</param>
  /// <param name="aValue">The value</param>
  /// <returns>A pair containing an Iterator (can be end) and a bool result, true if operation has succeded.</returns>
  std::pair<iterator, bool> insert (const KeyT &aKey, const ValueT &aValue);

  /// <summary>Finds an element with a key in the map.</summary>
  /// <param name="aKey">The key</param>
  /// <returns>Iterator to the found element (will be end() if key is not found).</returns>
  iterator find (const KeyT &aKey);

  /// <summary>Finds an element with a key in the map.</summary>
  /// <param name="aKey">The key</param>
  /// <returns>Write-locked iterator to the found element (will be end() if key is not found).</returns>
  const iterator find (const KeyT &aKey) const;

  /// <summary>Erases the element pointed by the Iterator. Invalidates the Iterator</summary>
  /// <param name="anIterator">The Iterator</param>
  /// <returns>True if element was present in the map (IE Iterator was valid).</returns>
  bool erase (const iterator &anIterator);

  /// <summary>Erases the element with the key param. Invalidates any Iterator to this element.</summary>
  /// <param name="aKey">The key</param>
  /// <returns>True if element was present in the map.</returns>
  bool erase (const KeyT &aKey);

  /// <summary>Increases the number of buckets and starts moving all valid (not erased) to the new buckets.</summary>
  /// <param ></param>
  /// <returns></returns>
  void rehash ();

private:
  using InternalValue = internal_value<KeyT, ValueT, HashFuncT>;
  using Bucket = bucket<KeyT, ValueT, HashFuncT>;

private:
  std::size_t getNextPopulatedBucketIndex (std::size_t anIndex) const;
  SharedVariantLock aquireBucketLock (int bucketIndex) const;
  static SharedVariantLock getValueLockFor (std::shared_mutex *mutexAddress, LockType lockType);
  static SharedVariantLock getBucketLockFor (std::shared_mutex *mutexAddress, LockType lockType);
  static SharedVariantLock aquireLockFor (std::shared_mutex *mutexAddress, LockType lockType, LockMap &lockMap);

  /// <summary>Gets the key of the first element - equivalent to begin()</summary>
  /// <param></param>
  /// <returns></returns>
  KeyT getFirstKey () const;

  void advanceIterator (iterator &it) const;

  void lockResource (std::size_t &bucketIndex, int &valueIndex) const;

  void unlockResource (std::size_t &bucketIndex, int &valueIndex) const;

private:
  HashFuncT hashFunc;
  std::vector<Bucket> buckets;
  std::size_t currentBucketCount;
  std::atomic<uint64_t> valueCount;
  std::atomic<uint64_t> erasedCount;
  float erase_threshold;

  friend iterator;
  friend InternalValue;
  friend Bucket;
};

template <class KeyT, class ValueT, class HashFuncT>
concurrent_unordered_map<KeyT, ValueT, HashFuncT>::concurrent_unordered_map (std::size_t bucketCount,
									     float erase_threshold_value)
{
  buckets.resize (bucketCount);
  currentBucketCount = bucketCount;
  valueCount = 0;
  erasedCount = 0;
  erase_threshold = erase_threshold_value;
}

template <class KeyT, class ValueT, class HashFuncT>
std::size_t
concurrent_unordered_map<KeyT, ValueT, HashFuncT>::size () const
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
	}
    }
  return end ();
}

template <class KeyT, class ValueT, class HashFuncT>
typename concurrent_unordered_map<KeyT, ValueT, HashFuncT>::iterator
concurrent_unordered_map<KeyT, ValueT, HashFuncT>::end () const
{
  return Iterator (this, true /*isEnd*/);
}

template <class KeyT, class ValueT, class HashFuncT>
std::pair<typename concurrent_unordered_map<KeyT, ValueT, HashFuncT>::iterator, bool>
concurrent_unordered_map<KeyT, ValueT, HashFuncT>::insert (const std::pair<KeyT, ValueT> &aKeyValuePair)
{
  auto hashResult = hashFunc (aKeyValuePair.first);
  int bucketIndex = int (hashResult) % currentBucketCount;

  auto result = buckets[bucketIndex].insert (this, bucketIndex, aKeyValuePair);
  if (result.second)
    {
      ++valueCount;
    }

  return result;
}

template <class KeyT, class ValueT, class HashFuncT>
std::pair<typename concurrent_unordered_map<KeyT, ValueT, HashFuncT>::iterator, bool>
concurrent_unordered_map<KeyT, ValueT, HashFuncT>::insert (const KeyT &aKey, const ValueT &aValue)
{
  auto hashResult = hashFunc (aKey);
  int bucketIndex = int (hashResult) % currentBucketCount;

  auto result = buckets[bucketIndex].insert (this, bucketIndex, std::make_pair (aKey, aValue));
  if (result.second)
    {
      ++valueCount;
    }

  return result;
}

template <class KeyT, class ValueT, class HashFuncT>
typename concurrent_unordered_map<KeyT, ValueT, HashFuncT>::iterator const
concurrent_unordered_map<KeyT, ValueT, HashFuncT>::find (const KeyT &aKey) const
{
  auto hashResult = hashFunc (aKey);
  int bucketIndex = int (hashResult) % currentBucketCount;

  return buckets[bucketIndex].find (this, bucketIndex, aKey, LockType::READ);
}

template <class KeyT, class ValueT, class HashFuncT>
typename concurrent_unordered_map<KeyT, ValueT, HashFuncT>::iterator
concurrent_unordered_map<KeyT, ValueT, HashFuncT>::find (const KeyT &aKey)
{
  auto hashResult = hashFunc (aKey);
  int bucketIndex = int (hashResult) % currentBucketCount;

  return buckets[bucketIndex].find (this, bucketIndex, aKey, LockType::WRITE);
}

template <class KeyT, class ValueT, class HashFuncT>
bool
concurrent_unordered_map<KeyT, ValueT, HashFuncT>::erase (const iterator &anIterator)
{
  return erase (anIterator.key);
}

template <class KeyT, class ValueT, class HashFuncT>
bool
concurrent_unordered_map<KeyT, ValueT, HashFuncT>::erase (const KeyT &aKey)
{
  auto hashResult = hashFunc (aKey);
  auto bucketIndex = int (hashResult) % currentBucketCount;

  int position = buckets[bucketIndex].erase (aKey);

  if (position != -1)
    {
      ++erasedCount;
      buckets[bucketIndex].eraseUnavailableValues (this, bucketIndex, erase_threshold);
    }

  if (position != -1)
    {
      return true;
    }
  return false;
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
SharedVariantLock
concurrent_unordered_map<KeyT, ValueT, HashFuncT>::aquireBucketLock (int bucketIndex) const
{
  return getBucketLockFor (&(*buckets[bucketIndex].bucketMutex), LockType::READ);
}

template <class KeyT, class ValueT, class HashFuncT>
SharedVariantLock
concurrent_unordered_map<KeyT, ValueT, HashFuncT>::getValueLockFor (std::shared_mutex *mutexAddress, LockType lockType)
{
  static thread_local LockMap value_mutex_to_lock;

  auto it = value_mutex_to_lock.find (mutexAddress);

  // Reasons for using shared_ptr:
  // - it allows for a custom destructor which we need
  // - it allows for a weak_ptr (we don't need ownership in this map - object would not destroy itself)
  // - adrress multiple entrancies from the same thread (IE: same thread creates it for the same value multiple times)
  SharedVariantLock sharedVariantLock;
  bool lock_needs_to_change = false;
  if (it != value_mutex_to_lock.end ())
    {
      auto [weakVariantLock, valueLockType] = it->second;
      sharedVariantLock = weakVariantLock.lock ();
      if (valueLockType == lockType)
	{
	  return sharedVariantLock;
	}
      else
	{
	  if (lockType == LockType::READ)
	    {
	      return sharedVariantLock; // If we have Write lock, and Read lock is needed, we pass the existing Write
					// lock
	    }
	  *sharedVariantLock = VariantLock (); // This will release the lock and call the destructor
	  lock_needs_to_change = true;
	}
    }

  auto lock = aquireLockFor (mutexAddress, lockType, value_mutex_to_lock);
  // We change Read lock to Write lock for all iterators that reference the same variant
  if (lock_needs_to_change)
    {
      *sharedVariantLock = *lock;
    }
  return lock;
}

template <class KeyT, class ValueT, class HashFuncT>
SharedVariantLock
concurrent_unordered_map<KeyT, ValueT, HashFuncT>::getBucketLockFor (std::shared_mutex *mutexAddress, LockType lockType)
{
  static thread_local LockMap bucket_mutex_to_lock;

  SharedVariantLock sharedVariantLock;
  bool lock_needs_to_change = false;
  auto it = bucket_mutex_to_lock.find (mutexAddress);
  if (it != bucket_mutex_to_lock.end ())
    {
      auto [weakVariantLock, valueLockType] = it->second;
      sharedVariantLock = weakVariantLock.lock ();
      if (valueLockType == lockType)
	{
	  return sharedVariantLock;
	}
      else
	{
	  if (lockType == LockType::READ)
	    {
	      return sharedVariantLock;
	    }
	  *sharedVariantLock = VariantLock (); // This will release the lock and call the destructor
	  lock_needs_to_change = true;
	}
    }

  auto lock = aquireLockFor (mutexAddress, lockType, bucket_mutex_to_lock);
  // We change Read lock to Write lock for all iterators that reference the same variant
  if (lock_needs_to_change)
    {
      *sharedVariantLock = *lock;
    }
  return lock;
}

template <class KeyT, class ValueT, class HashFuncT>
SharedVariantLock
concurrent_unordered_map<KeyT, ValueT, HashFuncT>::aquireLockFor (std::shared_mutex *mutexAddress, LockType lockType,
								  LockMap &lockMap)
{
#ifdef ADD_PERFORMANCE_COUNTERS
  MutexAquireCounters counters;
  counters.startTimeAquire = std::chrono::steady_clock::now ();
  counters.lockType = lockType;
  counters.threadID = std::this_thread::get_id ();
#endif

  SharedVariantLock lock;
  if (lockType == LockType::READ)
    {
      auto sharedReadLock = SharedReadLock (new ReadLock (*mutexAddress), [&lockMap, mutexAddress] (auto *p) {
	lockMap.erase (mutexAddress);
	delete p;
      });
      lock = std::make_shared<VariantLock> (sharedReadLock);
    }
  else
    {
      auto sharedWriteLock = SharedWriteLock (new WriteLock (*mutexAddress), [&lockMap, mutexAddress] (auto *p) {
	lockMap.erase (mutexAddress);
	delete p;
      });
      lock = std::make_shared<VariantLock> (sharedWriteLock);
    }
  auto resultInsert = lockMap.insert (std::make_pair (mutexAddress, std::make_tuple (lock, lockType)));
  assert (resultInsert.second);

#ifdef ADD_PERFORMANCE_COUNTERS
  counters.endTimeAquire = std::chrono::steady_clock::now ();
  GlobalCounter::addMutexAquireCounters (counters);
#endif

  return lock;
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
void
concurrent_unordered_map<KeyT, ValueT, HashFuncT>::advanceIterator (iterator &it) const
{
  int nextBucketIndex = it.bucketIndex;

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
  while (!found && nextBucketIndex < int (buckets.size ()));

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
concurrent_unordered_map<KeyT, ValueT, HashFuncT>::rehash ()
{
  // TODO: Implement
}

#endif
