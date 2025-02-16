#include <vector>
#include <memory>
#include <unordered_map>
#include "treeNode.h"
#include "treeAlignment.h"
#include <limits>

// example: trace [a,c,c,d,g], splits [-1,2,4] yield [[],[a,c,c],[d,g]]
// put in tree alignment
std::vector<std::shared_ptr<std::vector<std::string>>> segmentTrace(const std::shared_ptr<std::vector<std::string>> trace, const std::vector<int> &splits)
{
    std::vector<std::shared_ptr<std::vector<std::string>>> traceSegments;
    traceSegments.reserve(splits.size());

    int start = 0;
    const auto &defaultSubtrace = std::make_shared<std::vector<std::string>>(); // Empty segment
    for (int index : splits)
    {
        if (index == -1 || start > index)
        {
            traceSegments.emplace_back(defaultSubtrace);
        }
        else
        {
            // TODO test
            traceSegments.emplace_back(std::make_shared<std::vector<std::string>>(trace->begin() + start, trace->begin() + index + 1));
            start = index + 1;
        }
    }
    return traceSegments;
}

// helper function to compute possible splits
void calculatePossibleSplits(
    // perhaps change name childPositions
    const std::vector<std::vector<int>> &childPositions,
    std::vector<int> currentSplit,
    int position,
    std::vector<std::vector<int>> &possibleSplits,
    const int lastPosition)
{
    // Full segment computed
    if (position == childPositions.size())
    {
        possibleSplits.push_back(currentSplit);
        return;
    }
    // Previous elements of segment are already the last position e.g. [4,4,..] for lastPosition == 4
    else if (!currentSplit.empty() && currentSplit.back() == lastPosition)
    {
        currentSplit.push_back(lastPosition);
        calculatePossibleSplits(childPositions, currentSplit, position + 1, possibleSplits, lastPosition);
        return;
    }
    // child has no activites that are in this trace
    // TODO add test case
    else if (childPositions[position].empty())
    {
        currentSplit.push_back(currentSplit.empty() ? -1 : currentSplit.back());
        calculatePossibleSplits(childPositions, currentSplit, position + 1, possibleSplits, lastPosition);
        return;
    }
    // There are positions for this child, and the segment is not full yet
    if (!currentSplit.empty() && currentSplit.size() < childPositions.size() - 1 && currentSplit.back() != -1)
    {
        currentSplit.push_back(currentSplit.back());
        calculatePossibleSplits(childPositions, currentSplit, position + 1, possibleSplits, lastPosition);
        currentSplit.pop_back();
    }

    for (const auto &element : childPositions[position])
    {
        // only look at childPositions that are bigger than the current one. we want [1,5,6] not this [1,5,3]
        if (!currentSplit.empty() && currentSplit.back() > element)
        {
            continue;
        }

        currentSplit.push_back(element);
        calculatePossibleSplits(childPositions, currentSplit, position + 1, possibleSplits, lastPosition);
        currentSplit.pop_back();
    }
}

// suggest where splitting the trace would make sense for children, returns splitting point
// [a,b,v,d,y,u] that is supposed to be split among two children
// it returns something like this  [0,5] = [[a], [b,v,d,y,u]] or [-1,5] = [[a, b,v,d,y,u]]
std::vector<std::vector<int>> generateSplits(const std::shared_ptr<TreeNode> &node, const std::shared_ptr<std::vector<std::string>> trace)
{
    std::unordered_map<std::string, std::string> activityChildMap;
    std::unordered_map<std::string, int> nodeIdPostionMap;
    std::vector<std::vector<int>> childPositions(node->getChildren().size());

    int count = 0;
    for (const auto &child : node->getChildren())
    {
        childPositions[count].push_back(-1);
        nodeIdPostionMap[child->getId()] = count;

        for (const auto &pair : child->getActivities())
        {
            activityChildMap[pair.first] = child->getId();
        }
        ++count;
    }

    for (size_t i = 0; i < trace->size(); ++i)
    {
        std::string activity = trace->at(i);
        std::string childId = activityChildMap[activity];

        // next activity is the same as current activity
        if (i + 1 < trace->size() && activityChildMap[trace->at(i + 1)] == childId)
        {
            continue;
        }

        int position = nodeIdPostionMap[childId];
        childPositions[position].push_back(static_cast<int>(i));
    }

    // we mustn't create a split that doesn't take the whole trace into account
    childPositions[count - 1] = {static_cast<int>(trace->size() - 1)};

    std::vector<std::vector<int>> splits;
    calculatePossibleSplits(childPositions, {}, 0, splits, trace->size() - 1);

    // std::cout << "splits: " << std::to_string(splits.size()) << std::endl;
    return splits;
}

int oldAlignSequence(std::shared_ptr<TreeNode> node, const std::shared_ptr<std::vector<std::string>> trace)
{
    // std::cout << "oldsequence" << std::endl;
    int minCosts = std::numeric_limits<int>::max();

    std::vector<std::vector<int>> splits = generateSplits(node, trace);

    for (const auto &split : splits)
    {
        int costs = 0;
        const auto &splittedSegments = segmentTrace(trace, split);
        const auto &children = node->getChildren();

        for (int i = 0; i < splittedSegments.size(); i++)
        {
            costs += dynAlign(children[i], splittedSegments[i]);
        }

        minCosts = std::min(minCosts, costs);
    }

    return minCosts;
}