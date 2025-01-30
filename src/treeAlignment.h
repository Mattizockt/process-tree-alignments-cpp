#ifndef TREEALIGNMENT_H
#define TREEALIGNMENT_H
#include <memory>
#include "treeNode.h"

int dynAlign(std::shared_ptr<TreeNode> node, const std::shared_ptr<std::vector<std::string>> trace);
std::vector<std::vector<int>> generateSplits(const std::shared_ptr<TreeNode> &node, const std::shared_ptr<std::vector<std::string>> trace);  

#endif // TREENODE_H