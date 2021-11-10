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

  friend bool
  operator== (const RandomAccessIteratorType &a, const RandomAccessIteratorType &b)
  {
    return a.key == b.key && a.map == b.map;
  };
  friend bool
  operator!= (const RandomAccessIteratorType &a, const RandomAccessIteratorType &b)
  {
    return a.key != b.key || a.map != b.map;
  };

private:
  using Map = ConcurrentHashMap<KeyT, ValueT, HashFuncT>;
  using FWIterator = ForwardIteratorType<KeyT, ValueT, HashFuncT>;

  RandomAccessIteratorType (KeyT aKey, Map *aMap) : key (aKey), map (aMap)
  {
  }

  KeyT key;
  Map *map;

  friend Map;
  friend FWIterator;
};

#endif