#ifndef TREEALIGNMENT_H
#define TREEALIGNMENT_H
#include <memory>
#include "treeNode.h"

std::vector<std::shared_ptr<std::vector<std::string>>> segmentTrace(const std::shared_ptr<std::vector<std::string>> trace, const std::vector<int> &splits);
std::vector<std::vector<int>> generateSplits(const std::shared_ptr<TreeNode> &node, const std::shared_ptr<std::vector<std::string>> trace);
int dynAlign(std::shared_ptr<TreeNode> node, const std::shared_ptr<std::vector<int>> trace);

#endif // TREENODE_H