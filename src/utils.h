#ifndef UTILS_H
#define UTILS_H

#include "treeNode.h"
#include <map>

struct PairHash {
    template <typename A, typename B>
    size_t operator()(const std::pair<A, B>& p) const {
        return std::rotl(std::hash<A>{}(p.first), 1) ^ std::hash<B>{}(p.second);
    }
};

std::string pruneInputTrace(std::shared_ptr<TreeNode> node, const std::shared_ptr<std::vector<std::string>>);
void printVector(const std::vector<std::string> &vec);
void printNestedVector(const std::vector<std::vector<int>> &vec);
void printNestedVector(const std::vector<std::shared_ptr<std::vector<std::string>>> &nestedVec);
std::shared_ptr<TreeNode> constructTree(
    const std::vector<std::pair<Operation, std::vector<std::shared_ptr<TreeNode>>>> &structure);

#endif // UTILS_H
