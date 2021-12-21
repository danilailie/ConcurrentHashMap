cmake_minimum_required (VERSION 3.8)

set(CMAKE_CXX_STANDARD 17)

project ("ConcurrentHashMap")

include_directories(inc)

set(SOURCES 
    src/large_object.cpp
    src/main.cpp
)

add_executable (ConcurrentHashMap ${SOURCES})

if(UNIX)
    target_link_libraries(ConcurrentHashMap pthread)
endif()