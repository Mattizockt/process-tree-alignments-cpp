#ifndef UTILS_H
#define UTILS_H

#include "treeNode.h"
#include <map>

std::string pruneInputTrace(std::shared_ptr<TreeNode> node, const std::shared_ptr<std::vector<std::string>>);
void printVector(const std::vector<std::string> &vec);
void printNestedVector(const std::vector<std::vector<int>> &vec);
void printNestedVector(const std::vector<std::shared_ptr<std::vector<std::string>>> &nestedVec);
std::shared_ptr<TreeNode> constructTree(
    const std::vector<std::pair<Operation, std::vector<std::shared_ptr<TreeNode>>>> &structure);
std::vector<std::shared_ptr<std::vector<std::string>>> segmentTrace(const std::shared_ptr<std::vector<std::string>>, const std::vector<int> &segment);

#endif // UTILS_H
