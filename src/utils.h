#ifndef UTILS_H
#define UTILS_H

#include "treeNode.h"
#include <map>

std::string pruneInputTrace(std::shared_ptr<TreeNode> node, std::string &trace);
void printNestedVector(const std::vector<std::vector<int>> &vec);
std::shared_ptr<TreeNode> constructTree(
    std::vector<std::pair<Operation, std::vector<std::shared_ptr<TreeNode>>>> structure);
std::vector<std::string> segmentTrace(const std::string &trace, const std::vector<int> &segment);

#endif // UTILS_H
