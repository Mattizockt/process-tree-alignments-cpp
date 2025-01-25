cmake_minimum_required(VERSION 3.10)
project(process-tree-alignments-cpp)

set(CMAKE_CXX_STANDARD 17)

add_executable(process-tree-alignments-cpp src/main.cpp src/TreeNode.cpp src/utils.cpp) 

include(FetchContent)
FetchContent_Declare(
    Catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG v3.4.0 
)
FetchContent_MakeAvailable(Catch2)

add_executable(unit-tests tests/test.cpp)
target_link_libraries(unit-tests PRIVATE Catch2::Catch2WithMain)

include(CTest) # Enable CTest for test integration
include(Catch) # Use Catch's CMake utilities
catch_discover_tests(unit-tests)
