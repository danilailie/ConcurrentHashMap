// ConcurrentHashMap.cpp : Defines the entry point for the application.
//

#include <assert.h>
#include <iostream>

#include "ConcurrentHashMap.hpp"

template <typename T> struct customHash
{
  std::size_t
  operator() (const T &value)
  {
    return value;
  }
};

int
main ()
{
  ConcurrentHashMap<int, int> myMap;
  auto iterator = myMap.insert (7, 7);

  auto value = *iterator;

  auto it2 = myMap.find (7);
  assert (it2 != myMap.end ());

  auto it3 = myMap.find (5);
  assert (it3 == myMap.end ());

  auto it5 = myMap.insert (5, 5);
  std::cout << (*it5).first << " " << (*it5).second << '\n';

  //   myMap.erase (it5);
  //   myMap.erase (5);

  auto it4 = myMap.begin ();
  std::cout << (*it4).first << " " << (*it4).second << '\n';

  myMap.rehash ();

  std::cout << (*it4).first << " " << (*it4).second << '\n';
  std::cout << (*it5).first << " " << (*it5).second << '\n';

  return 0;
}
