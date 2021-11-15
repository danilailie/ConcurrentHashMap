#ifndef _BUCKET_HPP_
#define _BUCKET_HPP_

#include <shared_mutex>
#include <vector>

#include "InternalValue.hpp"

template <class KeyT, class ValueT, class HashFuncT> class ConcurrentHashMap;

template <class KeyT, class ValueT, class HashFuncT> class Bucket
{
public:
  using InternalValue = InternalValueType<KeyT, ValueT, HashFuncT>;

  Bucket ()
  {
    bucketMutex = std::make_unique<std::shared_mutex> ();
  }

  std::size_t
  getSize ()
  {
    std::shared_lock<std::shared_mutex> lock (*bucketMutex);
    return currentSize;
  }

  KeyT
  getFirstKey ()
  {
    std::shared_lock<std::shared_mutex> lock (*bucketMutex);
    for (auto i = 0; i < values.size (); ++i)
      {
	if (values[i].isAvailable ())
	  {
	    return values[i].getKey ();
	  }
      }
    return -1;
  }

  KeyT
  getKeyAt (std::size_t index)
  {
    std::shared_lock<std::shared_mutex> lock (*bucketMutex);
    return values[index].getKey ();
  }

  bool
  insert (const KeyT &aKey, const ValueT &aValue)
  {
    std::unique_lock<std::shared_mutex> lock (*bucketMutex);

    std::size_t foundPosition = -1;
    for (auto i = 0; i < values.size (); ++i)
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
      }
    else
      {
	if (!values[foundPosition].isAvailable ())
	  {
	    values[foundPosition].setAvailable ();
	  }
	else
	  {
	    return false;
	  }
      }

    return true;
  }

  void
  insert (const std::pair<KeyT, ValueT> &aKeyValuePair)
  {
    insert (aKeyValuePair.first, aKeyValuePair.second);
  }

  KeyT
  find (const KeyT &aKey)
  {
    std::shared_lock<std::shared_mutex> lock (*bucketMutex);
    for (auto i = 0; i < values.size (); ++i)
      {
	if (values[i].compareKey (aKey))
	  {
	    return aKey;
	  }
      }
    return -1;
  }

  KeyT
  erase (const KeyT &aKey)
  {
    std::unique_lock<std::shared_mutex> lock (*bucketMutex);
    for (auto i = 0; i < values.size (); ++i)
      {
	if (values[i].compareKey (aKey))
	  {
	    values[i].erase ();
	    --currentSize;
	    return aKey;
	  }
      }
    return -1;
  }

  std::pair<KeyT, ValueT>
  getKeyValuePair (const KeyT &aKey)
  {
    std::shared_lock<std::shared_mutex> lock (*bucketMutex);
    for (auto i = 0; i < values.size (); ++i)
      {
	if (values[i].compareKey (aKey))
	  {
	    return values[i].getKeyValuePair ();
	  }
      }
    return std::pair<KeyT, ValueT> ();
  }

  std::size_t
  getFirstValueIndex () const
  {
    std::shared_lock<std::shared_mutex> lock (*bucketMutex);
    for (auto i = 0; i < values.size (); ++i)
      {
	if (values[i].isAvailable ())
	  {
	    return i;
	  }
      }
    return values.size ();
  }

  std::size_t
  getNextValueIndex (std::size_t index)
  {
    std::shared_lock<std::shared_mutex> lock (*bucketMutex);
    for (auto i = index + 1; i < values.size (); ++i)
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

    for (auto i = 0; i < values.size (); ++i)
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
  using Map = ConcurrentHashMap<KeyT, ValueT, HashFuncT>;

  std::unique_ptr<std::shared_mutex> bucketMutex;
  std::vector<InternalValue> values;
  std::size_t currentSize = 0;

  friend Map;
};

#endif
