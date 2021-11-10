#include "LargeObject.hpp"

uint32_t LargeObject::copyCount = 0;
std::mutex LargeObject::copyMutex;