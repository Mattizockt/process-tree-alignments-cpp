#include "treeNode.h"
#include "utils.h"
#include <memory>
#include <string>
#include <numeric>
#include <limits>
#include <vector>
#include <algorithm>
#include <iostream>


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

    return splits;
}

int dynAlign(std::shared_ptr<TreeNode> node, const std::shared_ptr<std::vector<std::string>> trace);

int dynAlignActivity(std::shared_ptr<TreeNode> node, std::shared_ptr<std::vector<std::string>> trace)
{
    const std::string &activity = node->getActivity();

    if (std::find(trace->begin(), trace->end(), activity) == trace->end())
    {
        return trace->size() + 1;
    }
    else
    {
        return trace->size() - 1;
    }
}

int dynAlignSilentActivity(std::shared_ptr<TreeNode> node, const std::shared_ptr<std::vector<std::string>> trace)
{
    return trace->size();
}

int dynAlignSequence(std::shared_ptr<TreeNode> node, const std::shared_ptr<std::vector<std::string>> trace)
{
    if (trace->size() == 0)
    {
        const auto &children = node->getChildren();
        return std::accumulate(children.begin(), children.end(), 0, [&trace](int sum, const auto &child)
                               { return sum + dynAlign(child, trace); });
    }

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

int dynAlignXor(std::shared_ptr<TreeNode> node, const std::shared_ptr<std::vector<std::string>> trace)
{
    int minCost = std::numeric_limits<int>::max();
    for (const auto &child : node->getChildren())
    {
        int cost = dynAlign(child, trace);
        if (cost == 0)
        {
            return cost;
        }
        minCost = std::min(minCost, cost);
    }
    return minCost;
}

int dynAlignParallel(std::shared_ptr<TreeNode> node, const std::shared_ptr<std::vector<std::string>> trace)
{
    const auto &children = node->getChildren();
    std::vector<std::shared_ptr<std::vector<std::string>>> subTraces(children.size());
    std::generate(subTraces.begin(), subTraces.end(), []
                  { return std::make_shared<std::vector<std::string>>(); });

    int unmatched = std::count_if(trace->begin(), trace->end(), [&](const std::string &activity)
                                  {
        for (size_t i = 0; i < children.size(); i++) {
            if (children[i]->getActivities().count(activity)) {
                subTraces[i]->push_back(activity);
                return false;
            }
        }
        return true; });

    int cost = std::accumulate(children.begin(), children.end(), 0,
                               [&, i = 0](int sum, const auto &child) mutable
                               {
                                   return sum + dynAlign(child, subTraces[i++]);
                               });

    return cost + unmatched;
}

int dynAlignLoop(std::shared_ptr<TreeNode> node, const std::shared_ptr<std::vector<std::string>> trace)
{
    auto &children = node->getChildren();

    if (children.size() != 2)
    {
        throw std::runtime_error("Loop node with id: " + node->getId() + " does not have exactly two children.");
    }

    std::vector<std::pair<int, int>> edges;
    int n = trace->size();
    for (int i = 0; i <= n; ++i)
    {
        for (int j = i; j <= n; ++j)
        {
            edges.emplace_back(i, j);
        }
    }

    // so that we can use dynAlign to calculate cost
    auto tempNode = std::make_shared<TreeNode>(SEQUENCE);
    tempNode->addChild(children[1]);
    tempNode->addChild(children[0]);

    std::unordered_map<std::pair<int, int>, int, PairHash> qrCosts;
    for (const auto &edge : edges)
    {
        if (edge.first == edge.second)
        {
            qrCosts[edge] = 0;
            continue;
        }
        // TODO use arrays later to make this operation more efficient
        auto subTrace = std::make_shared<std::vector<std::string>>(trace->begin() + edge.first, trace->begin() + edge.second);
        int cost = dynAlign(tempNode, subTrace);
        qrCosts[edge] = cost;
    }

    for (size_t index = 0; index < n; index++)
    {
        bool change = false;
        for (const auto &edge : edges)
        {
            if (qrCosts[edge] == 0)
            {
                continue;
            }

            int optimalCost = qrCosts[edge];
            for (size_t j = edge.first + 1; j < edge.second; j++)
            {
                int newCost = qrCosts[{edge.first, j}] + qrCosts[{j, edge.second}];
                if (newCost < optimalCost)
                {
                    optimalCost = newCost;
                    change = true;
                }
            }
            qrCosts[edge] = optimalCost;
        }
        if (!change)
        {
            break;
        }
    }

    std::unordered_map<int, int> rCosts(n);
    for (size_t i = 0; i <= n; i++)
    {
        rCosts[i] = dynAlign(children[0], std::make_shared<std::vector<std::string>>(trace->begin(), trace->begin() + i));
    }

    int minimalCosts = std::numeric_limits<int>::max();
    for (size_t i = 0; i <= n; i++)
    {
        int costs = rCosts[i] + qrCosts[{i, n}];
        if (costs < minimalCosts)   
        {
            minimalCosts = costs;
        }
    }

    return minimalCosts;
}

// TODO perhaps implement refactor tree function in the future to remove xor loops
// because child creation is expensive
int dynAlignXorLoop(std::shared_ptr<TreeNode> node, const std::shared_ptr<std::vector<std::string>> trace)
{
    auto &children = node->getChildren();
    auto tempXorNode = std::make_shared<TreeNode>(XOR);
    auto tempRedoLoop = std::make_shared<TreeNode>(REDO_LOOP);
    auto silentNode = std::make_shared<TreeNode>(SILENT_ACTIVITY);

    tempXorNode->addChild(children[0]);
    tempXorNode->addChild(children[1]);

    tempRedoLoop->addChild(tempXorNode);
    tempRedoLoop->addChild(silentNode);

    auto x = dynAlignLoop(tempRedoLoop, trace);
    return x;
}

int dynAlign(std::shared_ptr<TreeNode> node, const std::shared_ptr<std::vector<std::string>> trace)
{
    if (costTable.count(node->getId()) > 0)
    {
        if (costTable[node->getId()].count(trace) == 1)
        {
            return costTable[node->getId()][trace];
        }
    }

    int costs;
    switch (node->getOperation())
    {
    case SEQUENCE:
        costs = dynAlignSequence(node, trace);
        break;
    case PARALLEL:
        costs = dynAlignParallel(node, trace);
        break;
    case XOR:
        costs = dynAlignXor(node, trace);
        break;
    case REDO_LOOP:
        costs = dynAlignLoop(node, trace);
        break;
    case XOR_LOOP:
        costs = dynAlignXorLoop(node, trace);
        break;
    case ACTIVITY:
        costs = dynAlignActivity(node, trace);
        break;
    case SILENT_ACTIVITY:
        costs = dynAlignSilentActivity(node, trace);
        break;
    default:
        throw std::runtime_error("Unknown node operation: " + std::to_string(node->getOperation()));
    }

    costTable[node->getId()][trace] = costs;
    return costs;
}
