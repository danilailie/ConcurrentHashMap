#ifndef _ITERATOR_HPP_
#define _ITERATOR_HPP_

template <class KeyT, class ValueT, class HashFuncT> class ConcurrentHashMap;

template <class KeyT, class ValueT, class HashFuncT> class Iterator
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
  operator== (const Iterator &a, const Iterator &b)
  {
    return a.key == b.key && a.map == b.map;
  };
  friend bool
  operator!= (const Iterator &a, const Iterator &b)
  {
    return a.key != b.key || a.map != b.map;
  };

private:
  using ConcurrentHashMapT = ConcurrentHashMap<KeyT, ValueT, HashFuncT>;

  Iterator (KeyT aKey, ConcurrentHashMapT *aMap) : key (aKey), map (aMap)
  {
  }

  KeyT key;
  ConcurrentHashMapT *map;

  friend ConcurrentHashMapT;
};

#endif