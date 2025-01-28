#include "treeNode.h"
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
    auto loopChild = std::make_shared<TreeNode>(REDO_LOOP); // New loop child

    auto child3 = std::make_shared<TreeNode>(ACTIVITY, "a");
    auto child4 = std::make_shared<TreeNode>(ACTIVITY, "b");
    auto child5 = std::make_shared<TreeNode>(ACTIVITY, "c");
    auto child6 = std::make_shared<TreeNode>(ACTIVITY, "d");
    auto child7 = std::make_shared<TreeNode>(ACTIVITY, "e");
    auto child8 = std::make_shared<TreeNode>(ACTIVITY, "f"); // New activity child
    auto child9 = std::make_shared<TreeNode>(ACTIVITY, "g"); // New activity child

    child1->addChild(child3);
    child1->addChild(child4);
    child1->addChild(child7);

    child2->addChild(child5);
    child2->addChild(child6);

    loopChild->addChild(child8); // Adding new activity child to loop child
    loopChild->addChild(child9); // Adding new activity child to loop child

    root->addChild(child1);
    root->addChild(child2);
    root->addChild(loopChild); // Adding loop child to root

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
    // Full segment computed
    if (position == childPositionsMap.size())
    {
        possibleSplits.push_back(currentSegment);
        return;
    }
    // we already occupied all possible posittions of the segment
    else if ((currentSegment.size() > 0 && currentSegment.back() == lastIndex))
    {
        currentSegment.push_back(lastIndex);
        splitHelper(childPositionsMap, currentSegment, position + 1, possibleSplits, lastIndex);
        currentSegment.push_back(lastIndex);
    }
    // no positions for this child
    else if (childPositionsMap[position].empty())
    {
        // no predecessor
        if (!currentSegment.empty())
        {
            currentSegment.push_back(-1);
        }
        // predecessor
        else
        {
            currentSegment.push_back(currentSegment.back());
        }
        splitHelper(childPositionsMap, currentSegment, position + 1, possibleSplits, lastIndex);
    }
    // there are positions for this child and segment is not full yet
    else
    {
        if (currentSegment.size() > 0 && currentSegment.size() < childPositionsMap.size() - 1 && currentSegment.back() != -1)
        {
            currentSegment.push_back(currentSegment.back());
            splitHelper(childPositionsMap, currentSegment, position + 1, possibleSplits, lastIndex);
            currentSegment.pop_back();
        }

        for (const auto &element : childPositionsMap[position])
        {
            if (!currentSegment.empty() && currentSegment.back() > element)
            {
                continue;
            }

            currentSegment.push_back(element);
            splitHelper(childPositionsMap, currentSegment, position + 1, possibleSplits, lastIndex);
            currentSegment.pop_back();
        }
    }
}

// TODO problem: even though 1,2,5 and 1,3,5 might belong to the same children they are still outputted. perhaps make it to 1,3,5 in the future.
std::vector<std::vector<int>> possibleSplits(std::shared_ptr<TreeNode> node, const std::string &trace)
{
    std::map<char, int> letterChildMap;
    std::map<int, int> idToPosition;
    std::vector<std::vector<int>> childPositionsMap(node->getChildren().size());

    int count = 0;
    for (const auto &child : node->getChildren())
    {
        // skip condition
        childPositionsMap[count].push_back(-1);

        idToPosition[child->getId()] = count;

        for (const auto &pair : child->getLetters())
        {
            letterChildMap[pair.first[0]] = child->getId();
        }
        count++;
    }

    for (int i = 0; i < trace.size(); i++)
    {
        char letter = trace[i];
        int childId = letterChildMap[letter];

        if (trace.size() > i + 1)
        {
            char nextLetter = trace[i + 1];
            int nextChildId = letterChildMap[nextLetter];

            if (childId == nextChildId)
            {
                continue;
            }
        }

        int position = idToPosition[childId];
        childPositionsMap[position].push_back(i);
    }

    // perhaps handle case specially if there's -1 in the childPositionsmap and also 0


    childPositionsMap[count - 1].clear();
    childPositionsMap[count - 1].push_back(trace.size() - 1);

    std::vector<std::vector<int>> possibleSplits;
    splitHelper(childPositionsMap, std::vector<int>(), 0, possibleSplits, trace.size() - 1);

    // std::cout << std::endl;
    // std::cout << "Possible splits: " << std::endl;
    // printNestedVector(possibleSplits);

    return possibleSplits;
}
