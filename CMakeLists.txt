cmake_minimum_required(VERSION 3.10)
project(process-tree-alignments-cpp)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Define source files for main executable
set(SOURCE_FILES
    src/main.cpp
    src/treeNode.cpp
    src/treeAlignment.cpp
    src/utils.cpp
    src/parser.cpp
)

# Define source files for unit tests (exclude main.cpp)
set(TEST_FILES
    tests/alignTest.cpp
    src/treeNode.cpp
    src/treeAlignment.cpp
    src/utils.cpp
    src/parser.cpp
)

add_executable(process-tree-alignments-cpp ${SOURCE_FILES})

include(FetchContent)
FetchContent_Declare(
    Catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG v3.4.0
)
FetchContent_MakeAvailable(Catch2)

add_executable(unit-tests ${TEST_FILES})
target_link_libraries(unit-tests PRIVATE Catch2::Catch2WithMain)

include(CTest)
include(Catch)
catch_discover_tests(unit-tests)
