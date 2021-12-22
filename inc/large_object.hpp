#ifndef _LARGE_OBJECT_HPP_
#define _LARGE_OBJECT_HPP_

#include <mutex>
#include <vector>

class LargeObject
{
public:
  explicit LargeObject (int anIndex) : index (anIndex)
  {
    data.resize (10000);
  }

  LargeObject (const LargeObject &other)
  {
    std::unique_lock<std::mutex> lock (copyMutex);
    data = other.data;
    copyCount++;
  }

  static uint32_t
  getCopyCount ()
  {
    std::unique_lock<std::mutex> lock (copyMutex);
    return copyCount;
  }

  int
  getIndex () const
  {
    return index;
  }

private:
  std::vector<uint32_t> data;
  int index;

  static uint32_t copyCount;
  static std::mutex copyMutex;
};

#endif