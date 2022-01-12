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
    std::shared_lock<std::shared_mutex> lock (*bucketMutex);
    return currentSize;
  }

  KeyT
  getFirstKey (int &position) const
  {
    std::shared_lock<std::shared_mutex> lock (*bucketMutex);
    for (auto i = 0; i < values.size (); ++i)
      {
	if (values[i].isAvailable ())
	  {
	    position = i;
	    return values[i].getKey ();
	  }
      }

    position = int (values.size ());
    return InvalidKeyValue<KeyT> ();
  }

  KeyT
  getKeyAt (std::size_t index) const
  {
    std::shared_lock<std::shared_mutex> lock (*bucketMutex);
    return values[index].getKey ();
  }

  bool
  insert (const KeyT &aKey, const ValueT &aValue, int &position)
  {
    std::unique_lock<std::shared_mutex> lock (*bucketMutex);

    int foundPosition = -1;
    int insertPosition = -1;
    for (int i = 0; i < int (values.size ()); ++i)
      {
	if (values[i].getKey () == aKey)
	  {
	    foundPosition = i;
	  }
      }

    if (foundPosition == -1)
      {
	values.push_back (InternalValue (aKey, aValue));
	++currentSize;
	insertPosition = int (values.size ()) - 1;
	position = insertPosition;
	return true;
      }
    else
      {
	if (!values[foundPosition].isAvailable ())
	  {
	    values[foundPosition].setAvailable ();
	    values[foundPosition].updateValue (aValue);
	    insertPosition = foundPosition;
	    position = insertPosition;
	    return true;
	  }
	else
	  {
	    position = foundPosition;
	    return false;
	  }
      }
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

  std::pair<KeyT, ValueT>
  getKeyValuePair (const KeyT &aKey) const
  {
    std::shared_lock<std::shared_mutex> lock (*bucketMutex);
    for (std::size_t i = 0; i < values.size (); ++i)
      {
	if (values[i].compareKey (aKey))
	  {
	    return values[i].getKeyValuePair ();
	  }
      }
    return std::pair<KeyT, ValueT> ();
  }

  int
  getFirstValueIndex () const
  {
    std::shared_lock<std::shared_mutex> lock (*bucketMutex);
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
    auto bucketLock = std::make_shared<std::shared_lock<std::shared_mutex>> (*bucketMutex);

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
    std::shared_lock<std::shared_mutex> lock (*bucketMutex);
    int itBucketIndex = it.bucketIndex;

    if (itBucketIndex == currentBucketIndex) // get next valid iterator in current bucket (if exists)
      {
	int nextValueIndex = getNextValueIndex (it.valueIndex);

	if (nextValueIndex != -1)
	  {
	    values[nextValueIndex].updateIterator (it, currentBucketIndex, nextValueIndex);
	    return true;
	  }
	else // need to go to next bucket
	  {
	    return false;
	  }
      }
    else // need to return the first valid element in this bucket
      {
	int nextValueIndex = getNextValueIndex (-1);

	if (nextValueIndex == -1)
	  {
	    return false;
	  }

	values[nextValueIndex].updateIterator (it, currentBucketIndex, nextValueIndex);
	return true;
      }
    return false;
  }

  Iterator
  getIterator (Map const *const aMap, int bucketIndex, KeyT key, bool withBucketLock) const
  {
    auto sharedBucketLock = std::make_shared<std::shared_lock<std::shared_mutex>> (*bucketMutex);

    auto valueIndex = findKey (key);
    if (valueIndex != -1)
      {
	std::shared_ptr<std::shared_lock<std::shared_mutex>> bucketLock;
	if (withBucketLock)
	  {
	    bucketLock = sharedBucketLock;
	  }
	return values[valueIndex].getIterator (aMap, bucketIndex, valueIndex, bucketLock);
      }
    else
      {
	return aMap->end ();
      }
  }

  int
  getNextValueIndex (int index) const
  {
    std::shared_lock<std::shared_mutex> lock (*bucketMutex);
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
  int
  findKey (const KeyT &aKey) const
  {
    for (int i = 0; i < int (values.size ()); ++i)
      {
	if (values[i].compareKey (aKey))
	  {
	    return i;
	  }
      }
    return -1;
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
