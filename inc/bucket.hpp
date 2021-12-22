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
  getFirstKey (std::size_t &position) const
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

    position = values.size ();
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
  using Map = concurrent_unordered_map<KeyT, ValueT, HashFuncT>;

  std::unique_ptr<std::shared_mutex> bucketMutex;
  std::vector<InternalValue> values;
  std::size_t currentSize = 0;

  friend Map;
};

#endif
