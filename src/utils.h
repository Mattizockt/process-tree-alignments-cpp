#ifndef UTILS_H
#define UTILS_H

#include "treeNode.h"
#include <map>

struct PairHash
{
    template <typename A, typename B>
    size_t operator()(const std::pair<A, B> &p) const
    {
        return std::rotl(std::hash<A>{}(p.first), 1) ^ std::hash<B>{}(p.second);
    }
};

std::shared_ptr<std::vector<int>> pruneTrace(const std::vector<std::shared_ptr<TreeNode>> &nodes, std::span<const int> unprunedTrace);
std::shared_ptr<TreeNode> constructTree(const std::vector<std::pair<Operation, std::vector<std::shared_ptr<TreeNode>>>> &structure);
std::shared_ptr<std::vector<int>> createSubtrace(const std::shared_ptr<std::vector<int>> &trace, size_t i, size_t j);
std::string timeInMs();
std::string visualizeIntTrace(const std::vector<int> &vec);
std::string visualizeSpanTrace(std::span<const int> span);
void printNestedVector(const std::vector<std::shared_ptr<std::vector<std::string>>> &nestedVec);
void printNestedVector(const std::vector<std::vector<int>> &vec);
void printVector(const std::vector<std::string> &vec);

#endif // UTILS_H
