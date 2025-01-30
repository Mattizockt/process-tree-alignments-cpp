#include "treeNode.h"
#include "utils.h"
#include <memory>
#include <string>
#include <numeric>
#include <limits>
#include <vector>
#include <algorithm>
#include <iostream>

// Helper function to compute possible splits
void calculatePossibleSplits(
    const std::vector<std::vector<int>> &childPositions,
    std::vector<int> currentSegment,
    int position,
    std::vector<std::vector<int>> &possibleSplits,
    const int lastIndex)
{
    // Full segment computed
    if (position == childPositions.size())
    {
        possibleSplits.push_back(currentSegment);
        return;
    }
    // We already occupied all possible positions of the segment
    else if (!currentSegment.empty() && currentSegment.back() == lastIndex)
    {
        currentSegment.push_back(lastIndex);
        calculatePossibleSplits(childPositions, currentSegment, position + 1, possibleSplits, lastIndex);
        return;
    }
    // No positions for this child
    else if (childPositions[position].empty())
    {
        currentSegment.push_back(currentSegment.empty() ? -1 : currentSegment.back());
        calculatePossibleSplits(childPositions, currentSegment, position + 1, possibleSplits, lastIndex);
        return;
    }

    // There are positions for this child, and the segment is not full yet
    if (!currentSegment.empty() && currentSegment.size() < childPositions.size() - 1 && currentSegment.back() != -1)
    {
        currentSegment.push_back(currentSegment.back());
        calculatePossibleSplits(childPositions, currentSegment, position + 1, possibleSplits, lastIndex);
        currentSegment.pop_back();
    }

    for (const auto &element : childPositions[position])
    {
        if (!currentSegment.empty() && currentSegment.back() > element)
        {
            continue;
        }

        currentSegment.push_back(element);
        calculatePossibleSplits(childPositions, currentSegment, position + 1, possibleSplits, lastIndex);
        currentSegment.pop_back();
    }
}

std::vector<std::vector<int>> generateSplits(const std::shared_ptr<TreeNode> &node, const std::shared_ptr<std::vector<std::string>> trace)
{
    std::unordered_map<std::string, std::string> activityChildMap;
    std::unordered_map<std::string, int> idToPosition;
    std::vector<std::vector<int>> childPositions(node->getChildren().size());

    int count = 0;
    for (const auto &child : node->getChildren())
    {
        childPositions[count].push_back(-1);
        idToPosition[child->getId()] = count;

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

        if (i + 1 < trace->size() && activityChildMap[trace->at(i + 1)] == childId)
        {
            continue;
        }

        int position = idToPosition[childId];
        childPositions[position].push_back(static_cast<int>(i));
    }

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

    std::vector<std::vector<int>> segments = generateSplits(node, trace);

    for (const auto &split : segments)
    {
        int costs = 0;
        const auto &splittedTraces = segmentTrace(trace, split);
        const auto &children = node->getChildren();

        for (int i = 0; i < splittedTraces.size(); i++)
        {
            // TODO accumluate
            costs += dynAlign(children[i], splittedTraces[i]);
            std::cout << "";
        }
        if (costs < minCosts)
        {
            minCosts = costs;
        }
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
    for (auto &ptr : subTraces)
    {
        ptr = std::make_shared<std::vector<std::string>>();
    }

    int unmatched = 0;

    // if trace empty, no subtraces
    for (const auto &activity : (*trace))
    {
        bool matched = false;
        for (size_t i = 0; i < children.size(); i++)
        {
            if (children[i]->getActivities().count(activity) == 1)
            {
                subTraces[i]->push_back(activity);
                matched = true;
                break;
            }
        }
        if (!matched)
        {
            unmatched++;
        }
    }

    int cost = 0;
    for (size_t i = 0; i < children.size(); i++)
    {
        cost += dynAlign(children[i], subTraces[i]);
    }

    cost += unmatched;

    return cost;
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

    auto tempNode = std::make_shared<TreeNode>(SEQUENCE);
    tempNode->addChild(children[0]);
    tempNode->addChild(children[1]);

    // TODO later change to unordered map, right now doesn't work because pair can't be used as a key
    // use hash function
    std::map<std::pair<int, int>, int> qrCosts;
    for (const auto &pair : edges)
    {
        if (pair.first == pair.second)
        {
            qrCosts[pair] = 0;
            continue;
        }
        // use arrays later to make this operation more efficient
        auto subTrace = std::make_shared<std::vector<std::string>>(trace->begin() + pair.first, trace->begin() + pair.second);
        int cost = dynAlign(tempNode, subTrace);
        // TODO perhaps use some upper bound like in the demo
        qrCosts[pair] = cost;
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
            for (size_t j = edge.first + 1; j <= edge.second; j++)
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
        int costs = rCosts[i] + qrCosts[{i + 1, n}];
        if (costs < minimalCosts)
        {
            minimalCosts = costs;
        }
    }

    return minimalCosts;
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
