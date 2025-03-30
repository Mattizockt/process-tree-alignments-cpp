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

std::shared_ptr<std::vector<int>> pruneTrace(const std::vector<std::shared_ptr<TreeNode>> &nodes, const std::shared_ptr<std::vector<int>> unprunedTrace);
std::shared_ptr<TreeNode> constructTree(const std::vector<std::pair<Operation, std::vector<std::shared_ptr<TreeNode>>>> &structure);
std::string timeInMs();
std::string visualizeIntTrace(const std::vector<int> &vec);
void printNestedVector(const std::vector<std::shared_ptr<std::vector<std::string>>> &nestedVec);
void printNestedVector(const std::vector<std::vector<int>> &vec);
void printVector(const std::vector<std::string> &vec);

#endif // UTILS_H
