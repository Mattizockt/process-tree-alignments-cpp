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

// Create an example TreeNode structure
std::shared_ptr<TreeNode> createExample()
{
    auto root = std::make_shared<TreeNode>(SEQUENCE);

    auto child1 = std::make_shared<TreeNode>(PARALLEL);
    auto child2 = std::make_shared<TreeNode>(XOR);
    auto loopChild = std::make_shared<TreeNode>(REDO_LOOP);

    auto child3 = std::make_shared<TreeNode>(ACTIVITY, "a");
    auto child4 = std::make_shared<TreeNode>(ACTIVITY, "b");
    auto child5 = std::make_shared<TreeNode>(ACTIVITY, "c");
    auto child6 = std::make_shared<TreeNode>(ACTIVITY, "d");
    auto child7 = std::make_shared<TreeNode>(ACTIVITY, "e");
    auto child8 = std::make_shared<TreeNode>(ACTIVITY, "f");
    auto child9 = std::make_shared<TreeNode>(ACTIVITY, "g");

    // Build the tree structure
    child1->addChild(child3);
    child1->addChild(child4);
    child1->addChild(child7);

    child2->addChild(child5);
    child2->addChild(child6);

    loopChild->addChild(child8);
    loopChild->addChild(child9);

    root->addChild(child1);
    root->addChild(child2);
    root->addChild(loopChild);

    return root;
}

// Print a nested vector for debugging purposes
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

// Helper function to compute possible splits
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
    // We already occupied all possible positions of the segment
    else if (!currentSegment.empty() && currentSegment.back() == lastIndex)
    {
        currentSegment.push_back(lastIndex);
        splitHelper(childPositionsMap, currentSegment, position + 1, possibleSplits, lastIndex);
        return;
    }
    // No positions for this child
    else if (childPositionsMap[position].empty())
    {
        currentSegment.push_back(currentSegment.empty() ? -1 : currentSegment.back());
        splitHelper(childPositionsMap, currentSegment, position + 1, possibleSplits, lastIndex);
        return;
    }

    // There are positions for this child, and the segment is not full yet
    if (!currentSegment.empty() && currentSegment.size() < childPositionsMap.size() - 1 && currentSegment.back() != -1)
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

// Generate possible splits
std::vector<std::vector<int>> generateSplits(const std::shared_ptr<TreeNode> &node, const std::string &trace)
{
    std::unordered_map<char, int> letterChildMap;
    std::unordered_map<int, int> idToPosition;
    std::vector<std::vector<int>> childPositionsMap(node->getChildren().size());

    int count = 0;
    for (const auto &child : node->getChildren())
    {
        childPositionsMap[count].push_back(-1);
        idToPosition[child->getId()] = count;

        for (const auto &pair : child->getLetters())
        {
            letterChildMap[pair.first[0]] = child->getId();
        }
        ++count;
    }

    for (size_t i = 0; i < trace.size(); ++i)
    {
        char letter = trace[i];
        int childId = letterChildMap[letter];

        if (i + 1 < trace.size() && letterChildMap[trace[i + 1]] == childId)
        {
            continue;
        }

        int position = idToPosition[childId];
        childPositionsMap[position].push_back(static_cast<int>(i));
    }

    childPositionsMap[count - 1] = {static_cast<int>(trace.size() - 1)};

    std::vector<std::vector<int>> splits;
    splitHelper(childPositionsMap, {}, 0, splits, trace.size() - 1);

    return splits;
}

std::vector<std::string> segmentTrace(const std::string &trace, const std::vector<int> &segment)
{
    std::vector<std::string> segments;
    int start = 0;

    for (size_t i = 0; i < segment.size(); i++)
    {
        // TODO improve later, currently unelegant
        if (segment[i] == -1 || (start >= segment[i] && start != 0))
        {
            segments.push_back("");
        }
        else {
               segments.push_back(trace.substr(start, segment[i] - start + 1));
            start = segment[i] + 1;
        }
    }

    return segments;
}