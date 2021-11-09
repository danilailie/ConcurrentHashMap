#ifndef _FORWARD_ITERATOR_HPP_
#define _FORWARD_ITERATOR_HPP_

template <class KeyT, class ValueT, class HashFuncT> class ConcurrentHashMap;

template <class KeyT, class ValueT, class HashFuncT> class ForwardIteratorType
{
public:
  ForwardIteratorType ()
  {
  }

  ForwardIteratorType (const ForwardIteratorType &other)
  {
    map = other.map;
    bucketIndex = other.bucketIndex;
    valueIndex = other.valueIndex;
  }

  ForwardIteratorType &
  operator= (const ForwardIteratorType &other)
  {
    if (this == &other)
      {
	return this;
      }

    map = other.map;
    bucketIndex = other.bucketIndex;
    valueIndex = other.valueIndex;

    return *this;
  }

  std::pair<KeyT, ValueT>
  operator* ()
  {
    return map->getIterValue (*this);
  }

private:
private:
  using Map = ConcurrentHashMap<KeyT, ValueT, HashFuncT>;
  Map *map;
  KeyT key; // used for compare between iterator types.
  std::size_t bucketIndex;
  std::size_t valueIndex;
};

#endif