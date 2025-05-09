cmake_minimum_required(VERSION 3.10)
project(process-tree-alignments-cpp)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# enable for profiling with perf
# set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pg")
# set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -pg")

add_definitions(-DPROJECT_SOURCE_DIR="${CMAKE_SOURCE_DIR}")
add_definitions(-DPROJECT_OUTPUT_DIR="${CMAKE_SOURCE_DIR}/output")

# Setup FetchContent once
include(FetchContent)

# Fetch pybind11
FetchContent_Declare(
  pybind11
  GIT_REPOSITORY https://github.com/pybind/pybind11.git
  GIT_TAG v2.11.1 # Use the latest stable version
)

# Fetch Catch2
FetchContent_Declare(
  Catch2
  GIT_REPOSITORY https://github.com/catchorg/Catch2.git
  GIT_TAG v3.4.0
)

# Fetch RapidXML
FetchContent_Declare(
    rapidxml
    GIT_REPOSITORY https://github.com/dwd/rapidxml.git
    GIT_TAG master
)

# Make RapidXML available
FetchContent_MakeAvailable(rapidxml)

# Make both available
FetchContent_MakeAvailable(pybind11 Catch2)

# Define common source files (excluding main.cpp and bindings.cpp)
set(COMMON_SOURCES
  src/treeNode.cpp
  src/treeAlignment.cpp
  src/utils.cpp
  src/parser.cpp
)

# Python module (without main.cpp)
pybind11_add_module(alignment
  src/bindings.cpp  # This should contain only your pybind11 bindings
  ${COMMON_SOURCES}
)

# Define source files for main executable
set(SOURCE_FILES
  src/main.cpp
  ${COMMON_SOURCES}
)

# Main executable
add_executable(process-tree-alignments-cpp ${SOURCE_FILES})
target_compile_definitions(process-tree-alignments-cpp PRIVATE
  ENABLE_UPPER_BOUND
)

# Test configuration
include(CTest)
include(Catch)