#ifndef _BUCKET_HPP_
#define _BUCKET_HPP_

#include <shared_mutex>
#include <vector>

#include "InternalValue.hpp"

template <class KeyT, class ValueT, class HashFuncT> class Bucket
{
public:
  using InternalValueT = InternalValue<KeyT, ValueT, HashFuncT>;

  Bucket ()
  {
    bucketMutex = std::make_unique<std::shared_mutex> ();
  }

  std::size_t
  getSize ()
  {
    std::size_t count = 0;
    std::shared_lock<std::shared_mutex> lock (*bucketMutex);
    for (auto i = 0; i < values.size (); ++i)
      {
	if (values[i].isAvailable ())
	  {
	    ++count;
	  }
      }
    return count;
  }

  KeyT
  getFirstKey ()
  {
    std::shared_lock<std::shared_mutex> lock (*bucketMutex);
    for (auto i = 0; i < values.size (); ++i)
      {
	if (values[i].isAvailable ())
	  {
	    return values[i].getKeyValuePair ().first;
	  }
      }
    return -1;
  }

  std::unique_ptr<std::shared_mutex> bucketMutex;
  std::vector<InternalValueT> values;
};

#endif
