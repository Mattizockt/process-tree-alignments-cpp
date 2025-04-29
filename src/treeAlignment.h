#ifndef TREEALIGNMENT_H
#define TREEALIGNMENT_H
#include "treeNode.h"
#include <memory>
#include <span>
#include <optional>

const size_t dynAlign(std::shared_ptr<TreeNode> node, const std::span<const int> trace);
extern std::optional<std::span<const int>> a;
#endif // TREENODE_H