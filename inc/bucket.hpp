#ifndef _BUCKET_HPP_
#define _BUCKET_HPP_

#include <shared_mutex>
#include <vector>

#include "internal_value.hpp"
#include "unordered_map_utils.hpp"

template <class KeyT, class ValueT, class HashFuncT> class concurrent_unordered_map;

template <class KeyT, class ValueT, class HashFuncT> class bucket
{
public:
  using InternalValue = internal_value<KeyT, ValueT, HashFuncT>;
  using Map = concurrent_unordered_map<KeyT, ValueT, HashFuncT>;
  using Iterator = typename concurrent_unordered_map<KeyT, ValueT, HashFuncT>::iterator;

  bucket ()
  {
    bucketMutex = std::make_unique<std::shared_mutex> ();
  }

  std::size_t
  getSize () const
  {
    auto bucketLock = Map::getBucketReadLockFor (&(*bucketMutex));
    return currentSize;
  }

  KeyT
  getFirstKey (int &position) const
  {
    auto bucketLock = Map::getBucketReadLockFor (&(*bucketMutex));
    for (auto i = 0; i < values.size (); ++i)
      {
	auto optionalKey = values[i].getKey ();
	if (optionalKey)
	  {
	    position = i;
	    return *optionalKey;
	  }
      }

    position = int (values.size ());
    return InvalidKeyValue<KeyT> ();
  }

  KeyT
  getKeyAt (std::size_t index) const
  {
    auto bucketLock = Map::getBucketReadLockFor (&(*bucketMutex));
    return values[index].getKey ();
  }

  std::pair<Iterator, bool>
  insert (Map const *const map, int bucketIndex, const std::pair<KeyT, ValueT> &aKeyValuePair,
	  bool isWriteLockedValue = false)
  {
    std::unique_lock<std::shared_mutex> lock (*bucketMutex);

    int foundPosition = -1;
    int insertPosition = -1;
    std::shared_ptr<std::shared_lock<std::shared_mutex>> emptyBucketMutex;

    for (int i = 0; i < int (values.size ()); ++i)
      {
	if (values[i].getKey () == aKeyValuePair.first)
	  {
	    foundPosition = i;
	  }
      }

    if (foundPosition != -1 && values[foundPosition].isAvailable ()) // there is a value with this key available
      {
	auto it = values[foundPosition].getIterator (map, bucketIndex, foundPosition, emptyBucketMutex, true);
	return std::make_pair (it, false);
      }

    if (foundPosition == -1) // key was not found
      {
	values.push_back (InternalValue (aKeyValuePair));
	++currentSize;
	insertPosition = int (values.size ()) - 1;
      }

    if (foundPosition != -1 && !values[foundPosition].isAvailable ()) // key was found, but previously erased.
      {
	values[foundPosition].setAvailable ();
	values[foundPosition].updateValue (aKeyValuePair.second);
	insertPosition = foundPosition;
      }

    auto it = values[insertPosition].getIterator (map, bucketIndex, insertPosition, emptyBucketMutex, true);
    return std::make_pair (it, true);
  }

  int
  erase (const KeyT &aKey)
  {
    std::unique_lock<std::shared_mutex> lock (*bucketMutex);
    for (int i = 0; i < int (values.size ()); ++i)
      {
	if (values[i].compareKey (aKey))
	  {
	    values[i].erase ();
	    --currentSize;
	    return i;
	  }
      }
    return -1;
  }

  int
  getFirstValueIndex () const
  {
    auto bucketLock = Map::getBucketReadLockFor (&(*bucketMutex));
    for (int i = 0; i < int (values.size ()); ++i)
      {
	if (values[i].isAvailable ())
	  {
	    return i;
	  }
      }
    return int (values.size ());
  }

  Iterator
  begin (Map const *const aMap, int bucketIndex) const
  {
    auto bucketLock = Map::getBucketReadLockFor (&(*bucketMutex));

    for (int i = 0; i < int (values.size ()); ++i)
      {
	if (values[i].isAvailable ())
	  {
	    return values[i].getIterator (aMap, bucketIndex, i, bucketLock);
	  }
      }

    return aMap->end ();
  }

  bool
  advanceIterator (Iterator &it, int currentBucketIndex) const
  {
    int itBucketIndex = it.bucketIndex;

    if (itBucketIndex == currentBucketIndex) // get next valid iterator in current bucket (if exists)
      {
	int nextValueIndex = getNextValueIndex (it.valueIndex);

	if (nextValueIndex != -1)
	  {
	    values[nextValueIndex].updateIterator (it, currentBucketIndex, nextValueIndex, it.bucketLock);
	    return true;
	  }
	else // need to go to next bucket
	  {
	    return false;
	  }
      }
    else // need to return the first valid element in this bucket
      {
	auto bucketLock = Map::getBucketReadLockFor (&(*bucketMutex));
	int nextValueIndex = getNextValueIndex (-1);

	if (nextValueIndex == -1)
	  {
	    return false;
	  }

	values[nextValueIndex].updateIterator (it, currentBucketIndex, nextValueIndex, bucketLock);
	return true;
      }
    return false;
  }

  Iterator
  find (Map const *const map, int bucketIndex, KeyT key, bool withBucketLock, ValueLockType valueLockType) const
  {
    auto sharedBucketLock = Map::getBucketReadLockFor (&(*bucketMutex));
    std::shared_ptr<std::shared_lock<std::shared_mutex>> noBucketLock;
    auto bucketLock = withBucketLock ? sharedBucketLock : noBucketLock;

    for (int i = 0; i < int (values.size ()); ++i)
      {
	auto result = values[i].getIteratorForKey (map, key, bucketIndex, i, bucketLock, valueLockType);
	if (result)
	  {
	    return *result;
	  }
      }

    return map->end ();
  }

  int
  getNextValueIndex (int index) const
  {
    auto valueLock = Map::getBucketReadLockFor (&(*bucketMutex));
    for (int i = index + 1; i < int (values.size ()); ++i)
      {
	if (values[i].isAvailable ())
	  {
	    return i;
	  }
      }
    return -1;
  }

private:
  void
  add (const std::pair<KeyT, ValueT> &aKeyValuePair)
  {
    values.push_back (InternalValue (aKeyValuePair.first, aKeyValuePair.second));
    ++currentSize;
  }

  std::size_t
  eraseUnavailableValues ()
  {
    std::unique_lock<std::shared_mutex> lock (*bucketMutex);
    std::vector<InternalValue> newValues;
    std::size_t count = 0;

    for (std::size_t i = 0; i < values.size (); ++i)
      {
	if (values[i].isAvailable ())
	  {
	    newValues.push_back (InternalValue (values[i].getKeyValuePair ()));
	    count++;
	  }
      }
    values = std::move (newValues);
    return count;
  }

private:
  std::unique_ptr<std::shared_mutex> bucketMutex;
  std::vector<InternalValue> values;
  std::size_t currentSize = 0;

  friend Map;
};

#endif
