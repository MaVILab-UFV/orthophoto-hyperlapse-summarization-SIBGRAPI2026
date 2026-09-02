# CMake version
cmake_minimum_required(VERSION 3.10)

# Project name
project(SemanticProfiler CXX)

# Set C++ standard (C++17 for std::filesystem)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find the GDAL library
find_package(GDAL REQUIRED)

# Tell the compiler to look for headers in the 'include' directory
include_directories(include ${GDAL_INCLUDE_DIRS})

# Define our source files
set(PROFILER_SOURCES
    src/main.cpp
    src/SemanticPipeline.cpp
    src/Heuristics.cpp
    src/ProfileGenerators.cpp
)

# Create the executable named 'pipeline'
add_executable(
    pipeline
    ${PROFILER_SOURCES}
)

# Link the 'pipeline' executable against the GDAL libraries
target_link_libraries(pipeline ${GDAL_LIBRARIES})