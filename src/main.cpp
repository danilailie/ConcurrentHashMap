// ConcurrentHashMap.cpp : Defines the entry point for the application.
//

#include <assert.h>
#include <chrono>
#include <iostream>
#include <memory>
#include <thread>
#include <unordered_map>

#include "ConcurrentHashMap.hpp"
#include "ForwardIterator.hpp"
#include "LargeObject.hpp"

int
main ()
{
  ConcurrentHashMap<int, std::shared_ptr<LargeObject>> myMap (10007);
  //   ConcurrentHashMap<int, int> myMap;

  //   for (auto i = 0; i < 100; ++i)
  //     {
  //       myMap.insert (i, std::make_shared<LargeObject> (i));
  //     }

  //   std::cout << "\nCopy count untill now: " << LargeObject::getCopyCount () << '\n';

  //   for (auto i = 0; i < 100; ++i)
  //     {
  //       auto it = myMap.find (i);
  //       assert (it != myMap.end ());
  //       std::cout << (*it).second->getIndex () << " ";
  //     }

  //   std::cout << "\nCopy count untill now: " << LargeObject::getCopyCount () << '\n';

  //   for (auto it = myMap.begin (); it != myMap.end (); ++it)
  //     {
  //       std::cout << (*it).second->getIndex () << " ";
  //     }

  //   std::cout << "\nCopy count untill now: " << LargeObject::getCopyCount () << '\n';

  auto populateFunc = [&myMap] (int left, int right) {
    for (auto i = left; i < right; ++i)
      {
	myMap.insert (i, std::make_shared<LargeObject> (i));
      }
  };

  std::vector<std::thread> workers;
  const int tenK = 10000;

  auto startTime = std::chrono::steady_clock::now ();

  for (auto i = 0; i < 10; ++i)
    {
      workers.push_back (std::thread ([populateFunc, i, tenK] () { populateFunc (i * tenK, (i + 1) * tenK); }));
    }

  for (auto &worker : workers)
    {
      worker.join ();
    }

  auto endTime = std::chrono::steady_clock::now ();

  //   for (auto it = myMap.begin (); it != myMap.end (); ++it)
  //     {
  //       std::cout << (*it).second /*->getIndex ()*/ << " ";
  //     }

  std::cout << "\nConcurrent Hash Map Duration: "
	    << std::chrono::duration_cast<std::chrono::milliseconds> (endTime - startTime).count ()
	    << " milliseconds\n";

  //   std::unordered_map<int, int> standardMap;
  std::unordered_map<int, std::shared_ptr<LargeObject>> standardMap;

  startTime = std::chrono::steady_clock::now ();

  for (auto i = 0; i < tenK * 10; ++i)
    {
      standardMap.insert (std::make_pair (i, std::make_shared<LargeObject> (i)));
    }

  endTime = std::chrono::steady_clock::now ();

  std::cout << "\nUnordered Map Duration: "
	    << std::chrono::duration_cast<std::chrono::milliseconds> (endTime - startTime).count ()
	    << " milliseconds\n";

  return 0;
}
