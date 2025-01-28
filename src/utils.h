#ifndef UTILS_H
#define UTILS_H

#include "treeProcessor.h"
#include <map>

std::string pruneInputTrace(std::shared_ptr<TreeNode> node, std::string &trace);
std::shared_ptr<TreeNode> createExample();
void printNestedVector(const std::vector<std::vector<int>> &vec);
std::vector<std::vector<int>> generateSplits(const std::shared_ptr<TreeNode> &node, const std::string &trace);

std::vector<std::string> segmentTrace(const std::string &trace, const std::vector<int> &segment);

#endif // UTILS_H
