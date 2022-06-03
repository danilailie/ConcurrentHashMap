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
#include "unordered_map_utils.hpp"

void
print (const std::string &aMessage, std::shared_mutex *mutex)
{
  static std::mutex coutMutex;
  auto lock = std::unique_lock<std::mutex> (coutMutex);
  std::cout << aMessage << " Mutex address: " << mutex << " Tread id: " << std::this_thread::get_id () << "\n";
}

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
  static SharedVariantLock getValueLockFor (std::shared_mutex *mutexAddress, ValueLockType lockType);
  static SharedVariantLock getBucketLockFor (std::shared_mutex *mutexAddress, ValueLockType lockType);

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
  return Iterator (InvalidKeyValue<KeyT> (), this, -1, -1);
}

template <class KeyT, class ValueT, class HashFuncT>
std::pair<typename concurrent_unordered_map<KeyT, ValueT, HashFuncT>::iterator, bool>
concurrent_unordered_map<KeyT, ValueT, HashFuncT>::insert (const std::pair<KeyT, ValueT> &aKeyValuePair)
{
  auto hashResult = hashFunc (aKeyValuePair.first);
  int bucketIndex = int (hashResult) % currentBucketCount;

  auto result = buckets[bucketIndex].insert (this, bucketIndex, aKeyValuePair, true);
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

  return buckets[bucketIndex].find (this, bucketIndex, aKey, false, ValueLockType::READ);
}

template <class KeyT, class ValueT, class HashFuncT>
typename concurrent_unordered_map<KeyT, ValueT, HashFuncT>::iterator
concurrent_unordered_map<KeyT, ValueT, HashFuncT>::find (const KeyT &aKey)
{
  auto hashResult = hashFunc (aKey);
  int bucketIndex = int (hashResult) % currentBucketCount;

  return buckets[bucketIndex].find (this, bucketIndex, aKey, false, ValueLockType::WRITE);
}

template <class KeyT, class ValueT, class HashFuncT>
bool
concurrent_unordered_map<KeyT, ValueT, HashFuncT>::erase (const iterator &anIterator)
{
  return erase ((*anIterator).first);
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
  return getBucketLockFor (&(*buckets[bucketIndex].bucketMutex), ValueLockType::READ);
}

template <class KeyT, class ValueT, class HashFuncT>
SharedVariantLock
concurrent_unordered_map<KeyT, ValueT, HashFuncT>::getValueLockFor (std::shared_mutex *mutexAddress,
								    ValueLockType lockType)
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
	  if (lockType == ValueLockType::READ)
	    {
	      return sharedVariantLock;
	    }
	  *sharedVariantLock = VariantLock ();
	  lock_needs_to_change = true;
	}
    }

  if (lockType == ValueLockType::READ)
    {
      auto sharedReadLock = SharedReadLock (new ReadLock (*mutexAddress), [mutexAddress] (auto *p) {
	value_mutex_to_lock.erase (mutexAddress);
	delete p;
      });
      auto lock = std::make_shared<VariantLock> (sharedReadLock);
      auto resultInsert = value_mutex_to_lock.insert (std::make_pair (mutexAddress, std::make_tuple (lock, lockType)));
      assert (resultInsert.second);
      return lock;
    }

  // this is the write case.
  auto sharedWriteLock = SharedWriteLock (new WriteLock (*mutexAddress), [mutexAddress] (auto *p) {
    value_mutex_to_lock.erase (mutexAddress);
    delete p;
  });
  auto lock = std::make_shared<VariantLock> (sharedWriteLock);
  if (lock_needs_to_change)
    {
      *sharedVariantLock = *lock;
    }
  auto resultInsert = value_mutex_to_lock.insert (std::make_pair (mutexAddress, std::make_tuple (lock, lockType)));
  assert (resultInsert.second);
  return lock;
}

template <class KeyT, class ValueT, class HashFuncT>
SharedVariantLock
concurrent_unordered_map<KeyT, ValueT, HashFuncT>::getBucketLockFor (std::shared_mutex *mutexAddress,
								     ValueLockType lockTypeRequested)
{
  static thread_local LockMap bucket_mutex_to_lock;

  SharedVariantLock sharedVariantLock;
  bool lock_needs_to_change = false;
  auto it = bucket_mutex_to_lock.find (mutexAddress);
  if (it != bucket_mutex_to_lock.end ())
    {
      auto [weakVariantLock, bucketLockType] = it->second;
      auto sharedVariantLock = weakVariantLock.lock ();

      assert (bucketLockType == lockTypeRequested);
      return sharedVariantLock;
    }

  if (lockTypeRequested == ValueLockType::READ)
    {
      auto sharedReadLock = SharedReadLock (new ReadLock (*mutexAddress), [mutexAddress] (auto *p) {
	print ("Delete read lock for mutex: ", mutexAddress);
	bucket_mutex_to_lock.erase (mutexAddress);
	delete p;
      });
      print ("Create read lock for mutex: ", mutexAddress);

      auto lock = std::make_shared<VariantLock> (sharedReadLock);
      auto resultInsert =
	bucket_mutex_to_lock.insert (std::make_pair (mutexAddress, std::make_tuple (lock, lockTypeRequested)));
      assert (resultInsert.second);
      return lock;
    }

  // this is the write case.
  auto sharedWriteLock = SharedWriteLock (new WriteLock (*mutexAddress), [mutexAddress] (auto *p) {
    print ("Delete write lock for mutex: ", mutexAddress);

    bucket_mutex_to_lock.erase (mutexAddress);
    delete p;
  });
  print ("Created write lock for mutex: ", mutexAddress);

  auto lock = std::make_shared<VariantLock> (sharedWriteLock);
  auto resultInsert =
    bucket_mutex_to_lock.insert (std::make_pair (mutexAddress, std::make_tuple (lock, lockTypeRequested)));
  assert (resultInsert.second);
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
