#ifndef _BUCKET_HPP_
#define _BUCKET_HPP_

#include <memory>
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
    auto bucketLock = Map::getBucketLockFor (&(*bucketMutex), LockType::READ);
    return currentSize;
  }

  std::pair<Iterator, bool>
  insert (Map const *const map, int bucketIndex, const std::pair<KeyT, ValueT> &aKeyValuePair)
  {
    auto bucketLock = Map::getBucketLockFor (&(*bucketMutex), LockType::WRITE);

    int foundPosition = -1;
    int insertPosition = -1;

    for (int i = 0; i < int (values.size ()); ++i)
      {
	auto key = values[i]->getKey ();
	if (key.has_value () && key.value () == aKeyValuePair.first)
	  {
	    foundPosition = i;
	  }
      }

    bool is_value_available = false;
    if (foundPosition != -1)
      {
	is_value_available = values[foundPosition]->isAvailable ();
      }

    if (foundPosition != -1 && is_value_available) // there is a value with this key available
      {
	auto it = values[foundPosition]->getIterator (map, bucketIndex, foundPosition, bucketLock, LockType::WRITE);
	return std::make_pair (it, false);
      }

    if (foundPosition == -1) // key was not found
      {
	values.push_back (std::make_shared<InternalValue> (aKeyValuePair));
	++currentSize;
	insertPosition = int (values.size ()) - 1;
      }

    if (foundPosition != -1 && !is_value_available) // key was found, but previously erased.
      {
	values[foundPosition]->updateValue (aKeyValuePair.second);
	insertPosition = foundPosition;
      }

    auto it = values[insertPosition]->getIterator (map, bucketIndex, insertPosition, bucketLock, LockType::WRITE);
    return std::make_pair (it, true);
  }

  int
  erase (const KeyT &aKey)
  {
    auto bucketLock = Map::getBucketLockFor (&(*bucketMutex), LockType::WRITE);
    for (int i = 0; i < int (values.size ()); ++i)
      {
	if (values[i]->compareKey (aKey))
	  {
	    values[i]->erase ();
	    --currentSize;
	    return i;
	  }
      }
    return -1;
  }

  Iterator
  begin (Map const *const aMap, int bucketIndex) const
  {
    auto bucketLock = Map::getBucketLockFor (&(*bucketMutex), LockType::READ);

    for (int i = 0; i < int (values.size ()); ++i)
      {
	if (values[i]->isAvailable ())
	  {
	    return values[i]->getIterator (aMap, bucketIndex, i, bucketLock, LockType::READ);
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
	    values[nextValueIndex]->updateIterator (it, currentBucketIndex, nextValueIndex, it.bucketLock);
	    return true;
	  }
	else // need to go to next bucket
	  {
	    return false;
	  }
      }
    else // need to return the first valid element in this bucket
      {
	using SharedReadLock = std::shared_ptr<std::shared_lock<std::shared_mutex>>;
	auto variantBucketLock = Map::getBucketLockFor (&(*bucketMutex), LockType::READ);
	int nextValueIndex = getNextValueIndex (-1);

	if (nextValueIndex == -1)
	  {
	    return false;
	  }

	values[nextValueIndex]->updateIterator (it, currentBucketIndex, nextValueIndex, variantBucketLock);
	return true;
      }
    return false;
  }

  Iterator
  find (Map const *const map, int bucketIndex, KeyT key, LockType lockType) const
  {
    auto bucketLock = Map::getBucketLockFor (&(*bucketMutex), lockType);

    for (int i = 0; i < int (values.size ()); ++i)
      {
	auto it = values[i]->getIteratorForKey (map, key, bucketIndex, i, bucketLock, lockType);
	if (it != map->end ())
	  {
	    return it;
	  }
      }

    return map->end ();
  }

  int
  getNextValueIndex (int index) const
  {
    auto valueLock = Map::getBucketLockFor (&(*bucketMutex), LockType::READ);
    for (int i = index + 1; i < int (values.size ()); ++i)
      {
	if (values[i]->isAvailable ())
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
  eraseUnavailableValues (Map const *const aMap, const int bucketIndex, const double threshold)
  {
    {
      auto bucketLock = Map::getBucketLockFor (&(*bucketMutex), LockType::READ);
      if (double (currentSize) > double (values.size ()) * threshold)
	{
	  return currentSize;
	}
    }

    auto bucketLock = Map::getBucketLockFor (&(*bucketMutex), LockType::WRITE);
    std::vector<std::shared_ptr<InternalValue>> newValues;
    std::size_t count = 0;

    for (std::size_t i = 0; i < values.size (); ++i)
      {
	if (values[i]->isAvailable ())
	  {
	    newValues.push_back (values[i]);
	    count++;
	  }
      }
    values = std::move (newValues);
    currentSize = count;
    return count;
  }

private:
  std::unique_ptr<std::shared_mutex> bucketMutex;
  std::vector<std::shared_ptr<InternalValue>> values;
  std::size_t currentSize = 0;

  friend Map;
};

#endif
