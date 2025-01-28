cmake_minimum_required(VERSION 3.10)
project(process-tree-alignments-cpp)

set(CMAKE_CXX_STANDARD 17)

add_executable(process-tree-alignments-cpp src/main.cpp src/treeNode.cpp src/utils.cpp) 

include(FetchContent)
FetchContent_Declare(
    Catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG v3.4.0 
)
FetchContent_MakeAvailable(Catch2)

add_executable(unit-tests tests/possibleSplitsTest.cpp src/treeNode.cpp src/utils.cpp) # Include necessary files
target_link_libraries(unit-tests PRIVATE Catch2::Catch2WithMain)

include(CTest)
include(Catch)
catch_discover_tests(unit-tests)
