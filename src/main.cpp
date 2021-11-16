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
  ConcurrentHashMap<int, std::shared_ptr<int>> myMap (10007);

  auto populateFunc = [&myMap] (int left, int right) {
    for (auto i = left; i < right; ++i)
      {
	myMap.insert (i, std::make_shared<int> (i));
      }
  };

  auto findFunc = [&myMap] (int left, int right) {
    for (auto i = left; i < right; ++i)
      {
	auto it = myMap.find (i);
	assert (it != myMap.end ());
      }
  };

  auto eraseFunc = [&myMap] (int left, int right) {
    for (auto i = left; i < right; ++i)
      {
	auto it = myMap.erase (i);
	assert (it != myMap.end ());
      }
  };

  auto traverseFunc = [&myMap] () {
    std::size_t valueCount = 0;
    for (auto it = myMap.begin (); it != myMap.end (); ++it)
      {
	++valueCount;
      }

    std::cout << "\nTraversed " << valueCount << " values.\n";
  };

  std::vector<std::thread> workers;
  const int tenK = 10000;

  auto startTimePopulate = std::chrono::steady_clock::now ();

  for (auto i = 0; i < 10; ++i)
    {
      workers.push_back (std::thread ([populateFunc, i, tenK] () { populateFunc (i * tenK, (i + 1) * tenK); }));
    }

  for (auto &worker : workers)
    {
      worker.join ();
    }

  auto endTimePopulate = std::chrono::steady_clock::now ();

  std::cout << "\nConcurrent Hash Map - Insert Duration: "
	    << std::chrono::duration_cast<std::chrono::milliseconds> (endTimePopulate - startTimePopulate).count ()
	    << " milliseconds\n";

  workers.clear ();

  auto rehashThread = std::thread ([&myMap] () { myMap.rehash (); });
  rehashThread.join ();

  auto startTimeFind = std::chrono::steady_clock::now ();

  for (auto i = 0; i < 10; ++i)
    {
      workers.push_back (std::thread ([findFunc, i, tenK] () { findFunc (i * tenK, (i + 1) * tenK); }));
    }

  for (auto &worker : workers)
    {
      worker.join ();
    }

  auto endTimeFind = std::chrono::steady_clock::now ();

  std::cout << "\nConcurrent Hash Map - Find Duration: "
	    << std::chrono::duration_cast<std::chrono::milliseconds> (endTimeFind - startTimeFind).count ()
	    << " milliseconds\n";

  workers.clear ();

  auto traverseThread = std::thread ([traverseFunc] () { traverseFunc (); });

  auto startTimeErase = std::chrono::steady_clock::now ();

  for (auto i = 0; i < 10; ++i)
    {
      workers.push_back (std::thread ([eraseFunc, i, tenK] () { eraseFunc (i * tenK, (i + 1) * tenK); }));
    }

  for (auto &worker : workers)
    {
      worker.join ();
    }

  auto endTimeErase = std::chrono::steady_clock::now ();

  traverseThread.join ();

  std::cout << "\nConcurrent Hash Map - Erase Duration: "
	    << std::chrono::duration_cast<std::chrono::milliseconds> (endTimeErase - startTimeErase).count ()
	    << " milliseconds\n";

  assert (myMap.erase (0) == myMap.end ());

  std::cout << "\nConcurrent Hash Map - Size: " << myMap.getSize () << '\n';

  //----------- Unordered map -----------//

  std::unordered_map<int, std::shared_ptr<int>> standardMap;

  auto startTimeSTD = std::chrono::steady_clock::now ();

  for (auto i = 0; i < tenK * 10; ++i)
    {
      standardMap.insert (std::make_pair (i, std::make_shared<int> (i)));
    }

  auto endTimeSTD = std::chrono::steady_clock::now ();

  std::cout << "\nUnordered Map - Insert Duration: "
	    << std::chrono::duration_cast<std::chrono::milliseconds> (endTimeSTD - startTimeSTD).count ()
	    << " milliseconds\n";

  auto startTimeFindSTD = std::chrono::steady_clock::now ();

  for (auto i = 0; i < tenK * 10; ++i)
    {
      auto it = standardMap.find (i);
      assert (it != standardMap.end ());
    }

  auto endTimeFindSTD = std::chrono::steady_clock::now ();

  std::cout << "\nUnordered Map - Find Duration: "
	    << std::chrono::duration_cast<std::chrono::milliseconds> (endTimeFindSTD - startTimeFindSTD).count ()
	    << " milliseconds\n";

  auto startTimeEraseSTD = std::chrono::steady_clock::now ();

  for (auto i = 0; i < tenK * 10; ++i)
    {
      auto it = standardMap.erase (i);
    }

  auto endTimeEraseSTD = std::chrono::steady_clock::now ();

  std::cout << "\nUnordered Map - Erase Duration: "
	    << std::chrono::duration_cast<std::chrono::milliseconds> (endTimeEraseSTD - startTimeEraseSTD).count ()
	    << " milliseconds\n";

  return 0;
}
