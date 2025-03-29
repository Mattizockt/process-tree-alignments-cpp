#include "treeNode.h"
#include <iostream>
#include <memory>
#include <unordered_set>
#include <unordered_map>
#include <vector>
#include <ctime>
#include <sstream>
#include "parser.h"

using IntVec = std::vector<int>;

// returns unix timestamp as string
std::string timeInMs()
{
    std::time_t currentTime = std::time(nullptr);
    std::string timeString = std::to_string(currentTime);
    return timeString;
}

/* inactive */

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

// std::string printDijsktraCosts(const std::unordered_map<std::pair<int, int>, int, PairHash> &dijkstraCosts)
// {
//     std::stringstream ss;
//     std::cout << std::endl;
//     ss << "{";

//     bool first = true;
//     for (const auto &entry : dijkstraCosts)
//     {
//         if (!first)
//         {
//             ss << ", ";
//         }
//         first = false;

//         ss << "(" << entry.first.first << ", " << entry.first.second << ")";
//         ss << ": " << entry.second;
//     }

//     ss << "}";
//     return ss.str();
// }

std::string intVecToString(const std::vector<int> &vec)
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
