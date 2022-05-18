#include <assert.h>
#include <chrono>
#include <iostream>
#include <memory>
#include <thread>
#include <unordered_map>
#include <utility>

#include "concurrent_unordered_map.hpp"
#include "iterator.hpp"
#include "large_object.hpp"

const int oneMill = 100000;
std::mutex stdMapMutex;

template <typename MapT>
void
insertInto (MapT &map, int left, int right, bool lock)
{
  for (auto i = left; i < right; ++i)
    {
      std::shared_ptr<std::unique_lock<std::mutex>> sharedLock;
      if (lock)
	{
	  sharedLock = std::make_shared<std::unique_lock<std::mutex>> (stdMapMutex);
	}
      map.insert (std::make_pair (i, std::make_shared<int> (i)));
    }
}

template <typename MapT>
void
findInto (MapT &map, int left, int right, bool lock)
{
  for (auto i = left; i < right; ++i)
    {
      std::shared_ptr<std::unique_lock<std::mutex>> sharedLock;
      if (lock)
	{
	  sharedLock = std::make_shared<std::unique_lock<std::mutex>> (stdMapMutex);
	}
      auto it = std::as_const (map).find (i);
      assert (it != map.end ());
    }
}

void
findIntoLock (concurrent_unordered_map<int, std::shared_ptr<int>> &map, int left, int right)
{
  for (auto i = left; i < right; ++i)
    {
      auto it = map.find (i);
      assert (it != map.end ());
    }
}

template <typename MapT>
void
timeInsertOperation (MapT &map, const std::string &mapType, bool lock)
{
  std::vector<std::thread> workers;
  auto startTimePopulate = std::chrono::steady_clock::now ();

  for (auto i = 0; i < int (std::thread::hardware_concurrency ()); ++i)
    {
      workers.push_back (std::thread ([&map, i, lock] () { insertInto (map, i * oneMill, (i + 1) * oneMill, lock); }));
    }

  for (auto &worker : workers)
    {
      worker.join ();
    }

  auto endTimePopulate = std::chrono::steady_clock::now ();
  std::cout << mapType << " - Insert Duration: "
	    << std::chrono::duration_cast<std::chrono::milliseconds> (endTimePopulate - startTimePopulate).count ()
	    << " milliseconds\n";
  workers.clear ();
}

template <typename MapT>
void
timeFindOperation (MapT &map, const std::string &mapType, bool lock)
{
  std::vector<std::thread> workers;
  auto startTime = std::chrono::steady_clock::now ();

  for (auto i = 0; i < int (std::thread::hardware_concurrency ()); ++i)
    {
      workers.push_back (std::thread ([&map, i, lock] () { findInto (map, i * oneMill, (i + 1) * oneMill, lock); }));
    }

  for (auto &worker : workers)
    {
      worker.join ();
    }

  auto endTime = std::chrono::steady_clock::now ();
  std::cout << mapType << " - Find Duration: "
	    << std::chrono::duration_cast<std::chrono::milliseconds> (endTime - startTime).count ()
	    << " milliseconds\n";
  workers.clear ();
}

void
timeFindLockOperation (concurrent_unordered_map<int, std::shared_ptr<int>> &map)
{
  std::vector<std::thread> workers;
  auto startTime = std::chrono::steady_clock::now ();

  for (auto i = 0; i < int (std::thread::hardware_concurrency ()); ++i)
    {
      workers.push_back (std::thread ([&map, i] () { findIntoLock (map, i * oneMill, (i + 1) * oneMill); }));
    }

  for (auto &worker : workers)
    {
      worker.join ();
    }

  auto endTime = std::chrono::steady_clock::now ();
  std::cout << "Concurrent Map"
	    << " - Find Lock Duration: "
	    << std::chrono::duration_cast<std::chrono::milliseconds> (endTime - startTime).count ()
	    << " milliseconds\n";
  workers.clear ();
}

template <typename MapT>
void
timeTraverseOperation (MapT &map, const std::string &mapType, bool lock)
{
  auto startTime = std::chrono::steady_clock::now ();

  std::shared_ptr<std::unique_lock<std::mutex>> sharedLock;
  if (lock)
    {
      sharedLock = std::make_shared<std::unique_lock<std::mutex>> (stdMapMutex);
    }

  std::size_t valueCount = 0;
  for (auto it = map.begin (); it != map.end (); ++it)
    {
      ++valueCount;
    }

  auto endTime = std::chrono::steady_clock::now ();
  std::cout << mapType << " - Traverse Duration: "
	    << std::chrono::duration_cast<std::chrono::milliseconds> (endTime - startTime).count ()
	    << " milliseconds. Value count: " << valueCount << "\n";
}

template <typename MapT>
void
eraseInto (MapT &map, int left, int right, bool lock)
{
  for (auto i = left; i < right; ++i)
    {
      std::shared_ptr<std::unique_lock<std::mutex>> sharedLock;
      if (lock)
	{
	  sharedLock = std::make_shared<std::unique_lock<std::mutex>> (stdMapMutex);
	}
      auto result = map.erase (i);
      assert (result);
    }
}

template <typename MapT>
void
timeEraseOperation (MapT &map, const std::string &mapType, bool lock)
{
  std::vector<std::thread> workers;
  auto startTime = std::chrono::steady_clock::now ();

  for (auto i = 0; i < int (std::thread::hardware_concurrency ()); ++i)
    {
      workers.push_back (std::thread ([&map, i, lock] () { eraseInto (map, i * oneMill, (i + 1) * oneMill, lock); }));
    }

  for (auto &worker : workers)
    {
      worker.join ();
    }

  auto endTime = std::chrono::steady_clock::now ();
  std::cout << mapType << " - Erase Duration: "
	    << std::chrono::duration_cast<std::chrono::milliseconds> (endTime - startTime).count ()
	    << " milliseconds\n";
  workers.clear ();
  assert (map.size () == 0);
}
int
main ()
{
  using namespace std::chrono_literals;
  //   std::cout << "Using " << std::thread::hardware_concurrency () << " threads...\n";
  //   concurrent_unordered_map<int, std::shared_ptr<int>> myMap (100003, 0.7);
  //   std::unordered_map<int, std::shared_ptr<int>> standardMap;

  //   timeInsertOperation (myMap, "Concurrent Map", false);
  //   timeInsertOperation (standardMap, "Standard Map", true);

  //   timeFindOperation (myMap, "Concurrent Map", false);
  //   timeFindOperation (standardMap, "Standard Map", true);
  //   timeFindLockOperation (myMap);

  //   timeTraverseOperation (myMap, "Concurrent Map", false);
  //   timeTraverseOperation (standardMap, "Standard Map", true);

  //   timeEraseOperation (myMap, "Concurrent Map", false);
  //   timeEraseOperation (standardMap, "Standard Map", true);

  //   myMap.insert (std::make_pair (7, std::make_shared<int> (7)));

  //   std::thread t ([&myMap] () {
  //     auto it = myMap.find (7);
  //     std::this_thread::sleep_for (4s);
  //     std::cout << "value: " << (*(it->second)) << '\n';
  //   });

  //   std::this_thread::sleep_for (1s);
  //   myMap.erase (7);

  //   t.join ();

  concurrent_unordered_map<int, uint64_t> myMap;
  std::vector<concurrent_unordered_map<int, uint64_t>::iterator> iterators;
  const uint64_t totalValues = 100000;

  std::cout << "Goint to insert...\n";
  for (auto i = 0; i < totalValues; ++i)
    {
      auto itBoolPair = myMap.insert (std::make_pair (i, i));
      assert (itBoolPair.second);
      iterators.push_back (itBoolPair.first);
    }
  std::cout << "Done.\n";
  std::cout << "Going to check iterators...\n";

  uint64_t valueCount = 0;
  for (auto i = 0; i < totalValues; ++i)
    {
      if (iterators[i]->second >= 0)
	{
	  ++valueCount;
	}
    }
  std::cout << "Done.\n";

  std::cout << "Total values counted: " << valueCount << '\n';

  return 0;
}
