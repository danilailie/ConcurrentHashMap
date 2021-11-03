// ConcurrentHashMap.cpp : Defines the entry point for the application.
//

#include <iostream>
#include "ConcurrentHashMap.h"

template <typename T>
struct customHash
{
  std::size_t operator () (const T& value)
  {
    return value;
  }
};

int main()
{
  ConcurrentHashMap<int, int> myMap;
  auto iterator = myMap.insert(7, 7);

  auto value = *iterator;

  return 0;
}
