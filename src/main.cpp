// ConcurrentHashMap.cpp : Defines the entry point for the application.
//

#include <assert.h>
#include <iostream>
#include <memory>

#include "ConcurrentHashMap.hpp"
#include "ForwardIterator.hpp"
#include "LargeObject.hpp"

int
main ()
{
  ConcurrentHashMap<int, std::shared_ptr<LargeObject>> myMap;

  for (auto i = 0; i < 100; ++i)
    {
      myMap.insert (i, std::make_shared<LargeObject> (i));
    }

  std::cout << "\nCopy count untill now: " << LargeObject::getCopyCount () << '\n';

  for (auto i = 0; i < 100; ++i)
    {
      auto it = myMap.find (i);
      assert (it != myMap.end ());
      std::cout << (*it).second->getIndex () << " ";
    }

  std::cout << "\nCopy count untill now: " << LargeObject::getCopyCount () << '\n';

  for (auto it = myMap.begin (); it != myMap.end (); ++it)
    {
      std::cout << (*it).second->getIndex () << " ";
    }

  std::cout << "\nCopy count untill now: " << LargeObject::getCopyCount () << '\n';

  return 0;
}
