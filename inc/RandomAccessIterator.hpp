#ifndef _ITERATOR_HPP_
#define _ITERATOR_HPP_

template <class KeyT, class ValueT, class HashFuncT> class ConcurrentHashMap;
template <class KeyT, class ValueT, class HashFuncT> class ForwardIteratorType;

template <class KeyT, class ValueT, class HashFuncT> class RandomAccessIteratorType
{
public:
  std::pair<KeyT, ValueT>
  operator* ()
  {
    return map->getIterValue (key);
  }

  KeyT
  getKey () const
  {
    return key;
  }

  RandomAccessIteratorType &
  operator= (const RandomAccessIteratorType &other)
  {
    if (this == &other)
      return *this;

    key = other.key;
    map = other.map;
  }

  bool
  operator== (const RandomAccessIteratorType &other)
  {
    return key == other.key && map == other.map;
  }

  bool
  operator!= (const RandomAccessIteratorType &other)
  {
    return key != other.key || map != other.map;
  }

private:
  using Map = ConcurrentHashMap<KeyT, ValueT, HashFuncT>;
  using FWIterator = ForwardIteratorType<KeyT, ValueT, HashFuncT>;

  RandomAccessIteratorType (KeyT aKey, Map const *const aMap) : key (aKey), map (aMap)
  {
  }

  KeyT key;
  const Map *map;

  friend Map;
  friend FWIterator;
};

#endif