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
  using ConcurrentHashMap = ConcurrentHashMap<KeyT, ValueT, HashFuncT>;
  using FWIterator = ForwardIteratorType<KeyT, ValueT, HashFuncT>;

  RandomAccessIteratorType (KeyT aKey, ConcurrentHashMap *aMap) : key (aKey), map (aMap)
  {
  }

  KeyT key;
  ConcurrentHashMap *map;

  friend ConcurrentHashMap;
  friend FWIterator;
};

#endif