#ifndef _ITERATOR_HPP_
#define _ITERATOR_HPP_

template< class KeyT, class ValueT, class HashFuncT>
class ConcurrentHashMap;

template< class KeyT, class ValueT, class HashFuncT>
class Iterator
{
public:
    std::pair<KeyT, ValueT> operator*()
    {
        return map->getIterValue(key);
    }
private:
    using ConcurrentHashMap = ConcurrentHashMap<KeyT, ValueT, HashFuncT>;

    Iterator(KeyT aKey, ConcurrentHashMap* aMap) : key(aKey), map(aMap) {}

    KeyT key;
    ConcurrentHashMap* map;

    friend class ConcurrentHashMap;
};

#endif