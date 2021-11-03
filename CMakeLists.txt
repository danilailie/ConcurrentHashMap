# CMakeList.txt : CMake project for ConcurrentHashMap, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.8)

project ("ConcurrentHashMap")

# Add source to this project's executable.
add_executable (ConcurrentHashMap "main.cpp" "ConcurrentHashMap.h")

# TODO: Add tests and install targets if needed.
