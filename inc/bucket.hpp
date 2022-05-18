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
    auto bucketLock = Map::getBucketLockFor (&(*bucketMutex), ValueLockType::READ);
    return currentSize;
  }

  std::pair<Iterator, bool>
  insert (Map const *const map, int bucketIndex, const std::pair<KeyT, ValueT> &aKeyValuePair,
	  bool isWriteLockedValue = false)
  {
    auto bucketLock = Map::getBucketLockFor (&(*bucketMutex), ValueLockType::WRITE);

    int foundPosition = -1;
    int insertPosition = -1;
    std::shared_ptr<std::shared_lock<std::shared_mutex>> emptyBucketMutex;

    for (int i = 0; i < int (values.size ()); ++i)
      {
	if (values[i]->getKey () == aKeyValuePair.first)
	  {
	    foundPosition = i;
	  }
      }

    if (foundPosition != -1 && values[foundPosition]->isAvailable ()) // there is a value with this key available
      {
	auto it = values[foundPosition]->getIterator (map, bucketIndex, foundPosition, emptyBucketMutex, true);
	return std::make_pair (it, false);
      }

    if (foundPosition == -1) // key was not found
      {
	values.push_back (std::make_shared<InternalValue> (aKeyValuePair));
	++currentSize;
	insertPosition = int (values.size ()) - 1;
      }

    if (foundPosition != -1 && !values[foundPosition]->isAvailable ()) // key was found, but previously erased.
      {
	values[foundPosition]->setAvailable ();
	values[foundPosition]->updateValue (aKeyValuePair.second);
	insertPosition = foundPosition;
      }

    auto it =
      values[insertPosition]->getIterator (map, bucketIndex, insertPosition, emptyBucketMutex, isWriteLockedValue);
    return std::make_pair (it, true);
  }

  int
  erase (const KeyT &aKey)
  {
    auto bucketLock = Map::getBucketLockFor (&(*bucketMutex), ValueLockType::WRITE);
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

  Iterator
  begin (Map const *const aMap, int bucketIndex) const
  {
    using SharedReadLock = std::shared_ptr<std::shared_lock<std::shared_mutex>>;
    auto variantBucketLock = Map::getBucketLockFor (&(*bucketMutex), ValueLockType::READ);
    auto bucketLock = std::get<SharedReadLock> (variantBucketLock);

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
	using SharedReadLock = std::shared_ptr<std::shared_lock<std::shared_mutex>>;
	auto variantBucketLock = Map::getBucketLockFor (&(*bucketMutex), ValueLockType::READ);
	auto bucketLock = std::get<SharedReadLock> (variantBucketLock);
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
    using SharedReadLock = std::shared_ptr<std::shared_lock<std::shared_mutex>>;

    auto variantBucketLock = Map::getBucketLockFor (&(*bucketMutex), ValueLockType::READ);
    auto readBucketLock = std::get<SharedReadLock> (variantBucketLock);

    std::shared_ptr<std::shared_lock<std::shared_mutex>> noBucketLock;
    auto bucketLock = withBucketLock ? readBucketLock : noBucketLock;

    for (int i = 0; i < int (values.size ()); ++i)
      {
	auto result = values[i]->getIteratorForKey (map, key, bucketIndex, i, bucketLock, valueLockType);
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
    auto valueLock = Map::getBucketLockFor (&(*bucketMutex), ValueLockType::READ);
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
  eraseUnavailableValues (Map const *const aMap, const int bucketIndex, const double threshold)
  {
    {
      auto bucketLock = Map::getBucketLockFor (&(*bucketMutex), ValueLockType::READ);
      if (double (currentSize) > double (values.size ()) * threshold)
	{
	  return currentSize;
	}
    }

    using SharedLock = std::shared_ptr<std::shared_lock<std::shared_mutex>>;
    auto bucketLock = Map::getBucketLockFor (&(*bucketMutex), ValueLockType::WRITE);
    std::vector<InternalValue> newValues;
    std::size_t count = 0;

    for (std::size_t i = 0; i < values.size (); ++i)
      {
	if (values[i].isAvailable ())
	  {
	    auto emptyBucketLock = SharedLock ();
	    auto valueIt = values[i].getIterator (aMap, bucketIndex, i, emptyBucketLock, true);
	    newValues.push_back (InternalValue (std::make_pair (valueIt->first, valueIt->second)));
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
