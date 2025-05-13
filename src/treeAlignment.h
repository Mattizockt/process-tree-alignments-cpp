#ifndef TREEALIGNMENT_H
#define TREEALIGNMENT_H
#include "treeNode.h"
#include <memory>
#include <span>
#include <atomic>

const size_t dynAlign(std::shared_ptr<TreeNode> node, const std::span<const int> trace);

extern std::atomic<bool> stop_flag;

#endif // TREENODE_H