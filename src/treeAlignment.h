#ifndef TREEALIGNMENT_H
#define TREEALIGNMENT_H
#include "treeNode.h"
#include <memory>
#include <span>

int dynAlign(std::shared_ptr<TreeNode> node, std::span<const int> trace);

#endif // TREENODE_H