#ifndef TREEALIGNMENT_H
#define TREEALIGNMENT_H
#include "treeNode.h"
#include <memory>
#include <span>

const size_t dynAlign(std::shared_ptr<TreeNode> node, const std::span<const int> trace);

#endif // TREENODE_H