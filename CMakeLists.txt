cmake_minimum_required(VERSION 3.10)
project(process-tree-alignments-cpp)

set(CMAKE_CXX_STANDARD 17)

add_executable(process-tree-alignments-cpp src/main.cpp src/TreeNode.cpp src/utils.cpp)