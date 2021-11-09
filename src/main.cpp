// ConcurrentHashMap.cpp : Defines the entry point for the application.
//

#include <assert.h>
#include <iostream>

#include "ConcurrentHashMap.hpp"
#include "ForwardIterator.hpp"

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
  //   auto iterator = myMap.insert (7, 7);

  //   auto value = *iterator;

  //   auto it2 = myMap.find (7);
  //   assert (it2 != myMap.end ());

  //   auto it3 = myMap.find (5);
  //   assert (it3 == myMap.end ());

  //   auto it5 = myMap.insert (5, 5);
  //   std::cout << (*it5).first << " " << (*it5).second << '\n';

  //   myMap.erase (it5);
  //   myMap.erase (5);

  //   auto it4 = myMap.begin ();
  //   std::cout << (*it4).first << " " << (*it4).second << '\n';

  //   myMap.rehash ();

  //   std::cout << (*it4).first << " " << (*it4).second << '\n';
  //   std::cout << (*it5).first << " " << (*it5).second << '\n';

  myMap.insert (1, 2);
  myMap.insert (2, 3);
  myMap.insert (3, 4);
  myMap.insert (4, 5);
  myMap.insert (5, 6);

  //   myMap.rehash ();

  for (auto it = myMap.begin (); it != myMap.end (); ++it)
    {
      std::cout << (*it).first << " " << (*it).second << "\n";
    }

  return 0;
}
