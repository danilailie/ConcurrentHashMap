cmake_minimum_required (VERSION 3.8)

set(CMAKE_CXX_STANDARD 17)

project ("ConcurrentHashMap")

include_directories(inc)

set(HEADERS
    inc/bucket.hpp
    inc/concurrent_unordered_map.hpp
    inc/forward_iterator.hpp
    inc/internal_value.hpp
    inc/unordered_map_utils.hpp
)

set(SOURCES 
    src/large_object.cpp
    src/main.cpp
)

add_executable (ConcurrentHashMap ${HEADERS} ${SOURCES})

if(UNIX)
    target_link_libraries(ConcurrentHashMap pthread)
endif()