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
  using iterator = typename concurrent_unordered_map<KeyT, ValueT, HashFuncT>::iterator;

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

  int
  insert (const KeyT &aKey, const ValueT &aValue)
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
      }
    else
      {
	if (!values[foundPosition].isAvailable ())
	  {
	    values[foundPosition].setAvailable ();
	    insertPosition = foundPosition;
	  }
      }
    return insertPosition;
  }

  int
  insert (const std::pair<KeyT, ValueT> &aKeyValuePair)
  {
    return insert (aKeyValuePair.first, aKeyValuePair.second);
  }

  int
  find (const KeyT &aKey) const
  {
    std::shared_lock<std::shared_mutex> lock (*bucketMutex);
    for (int i = 0; i < int (values.size ()); ++i)
      {
	if (values[i].compareKey (aKey))
	  {
	    return i;
	  }
      }
    return -1;
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

  iterator
  begin (Map const *const aMap, int bucketIndex) const
  {
    std::shared_lock<std::shared_mutex> lock (*bucketMutex);
    for (int i = 0; i < int (values.size ()); ++i)
      {
	if (values[i].isAvailable ())
	  {
	    return values[i].getIterator (aMap, bucketIndex, i);
	  }
      }

    return aMap->end ();
  }

  bool
  advanceIterator (iterator &it, std::size_t currentBucketIndex) const
  {
    std::shared_lock<std::shared_mutex> lock (*bucketMutex);
    auto itBucketIndex = it.bucketIndex;

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
	int nextValueIndex =
	  getNextValueIndex (-1); // we are sure there's at least one valid value in the current bucket.
	values[nextValueIndex].updateIterator (it, currentBucketIndex, nextValueIndex);
	return true;
      }
    return false;
  }

  iterator
  getIterator (Map const *const aMap, std::size_t bucketIndex, int valueIndex) const
  {
    std::shared_lock<std::shared_mutex> lock (*bucketMutex);
    return values[valueIndex].getIterator (aMap, bucketIndex, valueIndex);
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
