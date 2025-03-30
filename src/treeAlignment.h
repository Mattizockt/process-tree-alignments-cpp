#ifndef TREEALIGNMENT_H
#define TREEALIGNMENT_H
#include "treeNode.h"
#include <memory>

int dynAlign(std::shared_ptr<TreeNode> node, const std::shared_ptr<std::vector<int>> trace);

#endif // TREENODE_H