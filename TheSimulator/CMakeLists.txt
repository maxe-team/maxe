# CMakeList.txt : Top-level CMake project file, do global configuration
# and include sub-projects here.
#
cmake_minimum_required (VERSION 3.15)

project ("TheSimulator" VERSION 2.0.0 LANGUAGES CXX)

# C++ standard
set (CMAKE_CXX_STANDARD_REQUIRED ON)
set (CMAKE_CXX_STANDARD 17)

# Include sub-projects.
add_subdirectory ("TheSimulator")
