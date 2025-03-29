#include "treeNode.h"
#include <iostream>
#include <memory>
#include <unordered_set>
#include <unordered_map>
#include <vector>
#include <ctime>

// returns unix timestamp as string
std::string timeInMs()
{
    std::time_t currentTime = std::time(nullptr);
    std::string timeString = std::to_string(currentTime);
    return timeString;
}

/* inactive */
std::shared_ptr<std::vector<int>> pruneInputTrace(const std::shared_ptr<TreeNode> &node, const std::shared_ptr<std::vector<int>> trace)
{
    std::unordered_set<int> nodeLetters;
    for (const auto &pair : node->getActivities())
    {
        nodeLetters.insert(pair.first);
    }

    std::shared_ptr<std::vector<int>> prunedTrace;
    for (const auto &var : *trace)
    {
        if (nodeLetters.count(var))
        {
            prunedTrace->push_back(var);
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

void printNestedVector(const std::vector<std::shared_ptr<std::vector<std::string>>> &nestedVec)
{
    std::cout << "[\n";
    for (const auto vecPtr : nestedVec)
    {
        if (!vecPtr)
        {
            std::cout << "  null\n";
            continue;
        }

        std::cout << "  [ ";
        for (const auto &str : *vecPtr)
        {
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
