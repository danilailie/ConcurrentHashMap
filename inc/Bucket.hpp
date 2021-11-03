#ifndef _BUCKET_HPP_
#define _BUCKET_HPP_

#include <shared_mutex>
#include <vector>

#include "InternalValue.hpp"

template <class KeyT, class ValueT, class HashFuncT> class Bucket
{
public:
  using InternalValue = InternalValue<KeyT, ValueT, HashFuncT>;

  Bucket ()
  {
    bucketMutex = std::make_unique<std::shared_mutex> ();
  }

  std::size_t
  getSize ()
  {
    std::shared_lock<std::shared_mutex> lock (*bucketMutex);
    return values.size ();
  }

  KeyT
  getFirstKey ()
  {
    std::shared_lock<std::shared_mutex> lock (*bucketMutex);
    return values[0].getKeyValuePair ().first;
  }

  std::unique_ptr<std::shared_mutex> bucketMutex;
  std::vector<InternalValue> values;
};

#endif
