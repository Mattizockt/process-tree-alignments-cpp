#include "parser.h"
#include "treeNode.h"
#include <ctime>
#include <iostream>
#include <memory>
#include <sstream>
#include <unordered_map>
#include <unordered_set>
#include <vector>

using IntVec = std::vector<int>;

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

std::shared_ptr<std::vector<int>> createSubtrace(const std::shared_ptr<std::vector<int>> &trace, size_t i, size_t j)
{
    return std::make_shared<std::vector<int>>(trace->begin() + i, trace->begin() + j);
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

void printVector(const std::vector<std::string> &vec)
{
    for (const std::string &str : vec)
    {
        std::cout << str << ", ";
    }
    std::cout << std::endl;
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

std::shared_ptr<IntVec> pruneTrace(const std::vector<std::shared_ptr<TreeNode>> &nodes, const std::shared_ptr<IntVec> unprunedTrace)
{
    std::unordered_set<int> nodeLetters;
    for (const auto &node : nodes)
    {
        for (const auto &pair : node->getActivities())
        {
            nodeLetters.insert(pair.first);
        }
    }

    std::shared_ptr<IntVec> prunedTrace = std::make_shared<IntVec>();
    for (const auto &activity : *unprunedTrace)
    {
        if (nodeLetters.count(activity))
        {
            prunedTrace->push_back(activity);
        }
    }
    return prunedTrace;
}

// returns unix timestamp as string
std::string timeInMs()
{
    std::time_t currentTime = std::time(nullptr);
    std::string timeString = std::to_string(currentTime);
    return timeString;
}

std::string visualizeIntTrace(const std::vector<int> &vec)
{
    std::string result = "\"(";
    for (size_t i = 0; i < vec.size(); ++i)
    {
        result += "'";
        result += activityVector[vec[i]];
        result += "'";
        if (i < vec.size() - 1)
        {
            result += ", ";
        }
    }
    result += ")\"";
    return result;
}
