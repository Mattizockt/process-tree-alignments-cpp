#include "treeNode.h"
#include <iostream>
#include <memory>
#include <unordered_set>
#include <unordered_map>
#include <vector>

// Prune the input trace based on node activities
// TODO verify if correct
std::shared_ptr<std::vector<std::string>> pruneInputTrace(const std::shared_ptr<TreeNode> &node, const std::shared_ptr<std::vector<std::string>> trace)
{
    std::unordered_set<std::string> nodeLetters;
    for (const auto &pair : node->getActivities())
    {
        nodeLetters.insert(pair.first);
    }

    std::shared_ptr<std::vector<std::string>> prunedTrace;
    for (const auto &str : *trace)
    {
        if (nodeLetters.count(str))
        {
            prunedTrace->push_back(str);
        }
    }

    return prunedTrace;
}

void printVector(const std::vector<std::string> &vec)
{
    for (const std::string &str : vec)
    {
        std::cout << str << ", ";
    }
    std::cout << std::endl;
}

void printNestedVector(const std::vector<std::vector<int>> &vec)
{
    for (size_t i = 0; i < vec.size(); ++i)
    {
        std::cout << "Row " << i << ": ";
        for (int val : vec[i])
        {
            std::cout << val << " ";
        }
        std::cout << std::endl;
    }
}

// Function to print a nested vector of strings
void printNestedVector(const std::vector<std::shared_ptr<std::vector<std::string>>> &nestedVec) {
    std::cout << "[\n";
    for (const auto vecPtr : nestedVec) {
        if (!vecPtr) {
            std::cout << "  null\n";
            continue;
        }
        
        std::cout << "  [ ";
        for (const auto &str : *vecPtr) {
            std::cout << "\"" << str << "\" ";
        }
        std::cout << "]\n";
    }
    std::cout << "]\n";
}

std::shared_ptr<TreeNode> constructTree(
    const std::vector<std::pair<Operation, std::vector<std::shared_ptr<TreeNode>>>> &structure)
{
    auto root = std::make_shared<TreeNode>(SEQUENCE);

    std::vector<std::shared_ptr<TreeNode>> nodes;
    for (const auto &[op, children] : structure)
    {
        auto node = std::make_shared<TreeNode>(op);
        for (const auto &child : children)
        {
            node->addChild(child);
        }
        nodes.push_back(node);
    }

    for (const auto &node : nodes)
    {
        root->addChild(node);
    }

    root->fillActivityMaps();
    return root;
}
