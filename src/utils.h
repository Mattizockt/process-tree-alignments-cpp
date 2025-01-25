#ifndef UTILS_H
#define UTILS_H

#include "treeNode.h"
#include <map>


std::string pruneInputTrace(std::shared_ptr<TreeNode> node, std::string &trace);
std::shared_ptr<TreeNode> createExample();
void printNestedVector(const std::vector<std::vector<int>> &vec);
std::vector<std::vector<int>> possibleSplits(std::shared_ptr<TreeNode> node, const std::string &trace);

#endif // UTILS_H
