#include "TreeNode.h"
#include <iostream>
#include <memory>
#include <unordered_set>
#include <vector>
#include <map>

std::string pruneInputTrace(std::shared_ptr<TreeNode> node, std::string &trace)
{
    std::unordered_set<char> nodeLetters;
    for (const auto &pair : node->getLetters())
    {
        nodeLetters.insert(pair.first[0]);
    }

    std::string prunedTrace;
    for (char c : trace)
    {
        if (nodeLetters.count(c) > 0)
        {
            prunedTrace.push_back(c);
        }
    }

    return prunedTrace;
}

std::shared_ptr<TreeNode> createExample()
{
    auto root = std::make_shared<TreeNode>(SEQUENCE);

    auto child1 = std::make_shared<TreeNode>(PARALLEL);
    auto child2 = std::make_shared<TreeNode>(XOR);

    auto child3 = std::make_shared<TreeNode>(ACTIVITY, "a");
    auto child4 = std::make_shared<TreeNode>(ACTIVITY, "b");
    auto child5 = std::make_shared<TreeNode>(ACTIVITY, "c");
    auto child6 = std::make_shared<TreeNode>(ACTIVITY, "d");
    auto child7 = std::make_shared<TreeNode>(ACTIVITY, "e");

    child1->addChild(child3);
    child1->addChild(child4);
    child1->addChild(child7);

    child2->addChild(child5);
    child2->addChild(child6);

    root->addChild(child1);
    root->addChild(child2);

    return root;
}

void printNestedVector(const std::vector<std::vector<int>> &vec)
{
    for (size_t i = 0; i < vec.size(); i++)
    {
        std::cout << "Row " << i << ": ";
        for (int val : vec[i])
        {
            std::cout << val << " ";
        }
        std::cout << std::endl;
    }
}

void splitHelper(
    const std::vector<std::vector<int>> &childPositionsMap,
    std::vector<int> currentSegment,
    int position,
    std::vector<std::vector<int>> &possibleSplits,
    const int lastIndex)
{
    if (position == childPositionsMap.size())
    {
        if (currentSegment[position - 1] == lastIndex)
        {
            possibleSplits.push_back(currentSegment);
        }
        return;
    }

    for (const auto &element : childPositionsMap[position])
    {
        if (position > 0 && currentSegment[position - 1] > element)
        {
            continue;
        }

        currentSegment.push_back(element); 
        splitHelper(childPositionsMap, currentSegment, position + 1, possibleSplits, lastIndex);
        currentSegment.pop_back(); 
    }
}

std::vector<std::vector<int>> possibleSplits(std::shared_ptr<TreeNode> node, const std::string &trace)
{
    std::map<char, int> letterChildMap;
    std::map<int, int> idToPosition;
    std::vector<std::vector<int>> childPositionsMap;

    int count = 0;
    for (const auto &child : node->getChildren())
    {
        childPositionsMap.push_back(std::vector<int>());
        idToPosition[child->getId()] = count;
        for (const auto &pair : child->getLetters())
        {
            letterChildMap[pair.first[0]] = child->getId();
        }
        count++;
    }

    for (int i = 0; i < trace.size(); i++)
    {
        childPositionsMap[idToPosition[letterChildMap[trace[i]]]].push_back(i);
    }
    childPositionsMap[count-1].clear();
    childPositionsMap[count-1].push_back(trace.size()-1);

    std::vector<std::vector<int>> splits;
    splitHelper(childPositionsMap, std::vector<int>(), 0, splits, trace.size() - 1);

    std::cout << "Possible splits: " << std::endl;
    printNestedVector(splits);

    return splits;
}
