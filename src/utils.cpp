#include "treeNode.h"
#include <iostream>
#include <memory>
#include <unordered_set>
#include <unordered_map>
#include <vector>

// Prune the input trace based on node letters
std::string pruneInputTrace(const std::shared_ptr<TreeNode> &node, const std::string &trace)
{
    std::unordered_set<char> nodeLetters;
    for (const auto &pair : node->getLetters())
    {
        nodeLetters.insert(pair.first[0]);
    }

    std::string prunedTrace;
    prunedTrace.reserve(trace.size()); // Reserve memory to optimize performance
    for (char c : trace)
    {
        if (nodeLetters.count(c))
        {
            prunedTrace.push_back(c);
        }
    }

    return prunedTrace;
}

std::vector<std::string> segmentTrace(const std::string &trace, const std::vector<int> &segments)
{
    std::vector<std::string> result;
    result.reserve(segments.size()); // Reserve space for efficiency

    int start = 0;
    for (int index : segments)
    {
        if (index == -1 || start > index)
        {
            result.emplace_back(""); // Empty segment
        }
        else
        {
            result.emplace_back(trace.substr(start, index - start + 1));
            start = index + 1;
        }
    }
    return result;
}

void printVector(const std::vector<std::string> &vec)
{
    std::cout << "Vector contents:\n";
    for (const std::string &str : vec)
    {
        std::cout << str << std::endl;
    }
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

    root->fillLetterMaps();
    return root;
}
