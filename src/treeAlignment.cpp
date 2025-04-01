#include "treeNode.h"
#include "utils.h"
#include "parser.h"
#include <memory>
#include <string>
#include <numeric>
#include <limits>
#include <vector>
#include <algorithm>
#include <iostream>

using StringVec = std::vector<std::string>;
using IntVec = std::vector<int>;

// Forward declaration necessary in C++ (unlike Python where functions can be called before definition)
int dynAlign(std::shared_ptr<TreeNode> node, const std::shared_ptr<IntVec> trace);

// erase/insert is O(n) -> leaves us with O(trace->size() * n) complexity -> improve perhaps?
// can't be used with multithreading, in that case, trace must be changed first.
int estimateLowerBound(const std::shared_ptr<TreeNode> node, std::shared_ptr<IntVec> trace)
{
    int lowerBound = std::numeric_limits<int>::max();
    size_t n = trace->size();

    for (int i = 0; i < n; i++)
    {
        auto erased_val = trace->at(i);
        trace->erase(trace->begin() + i);

        std::string nodeId = node->getId();
        if (costTable.count(nodeId) > 0)
        {
            if (costTable[nodeId].count(*trace) == 1)
            {
                lowerBound = std::min(lowerBound, costTable[nodeId][*trace]);
            }
        }
        trace->insert(trace->begin() + i, erased_val);
    }
    if (lowerBound == std::numeric_limits<int>::max())
    {
        return 0;
    }
    else
    {
        // in case lower bound is 0
        // -1 because the best possible alignment cost improvement is 1
        return std::max(lowerBound - 1, 0);
    }
}

// Helper function to get segments - analogous to get_segments_for_sequence in Python
std::vector<std::pair<int, int>> getSegmentsForSequence(std::shared_ptr<IntVec> trace, std::shared_ptr<TreeNode> node)
{
    auto &children = node->getChildren();
    if (children.size() != 2)
    {
        throw std::runtime_error("get_segments_for_sequence not implemented for more/less than two children.");
    }

    int traceSize = trace->size();
    std::vector<std::pair<int, int>> segments = {
        {0, traceSize},
        {traceSize, 0}};

    std::vector<int> splitPositions;
    auto leftActivities = children[0]->getActivities();
    auto rightActivities = children[1]->getActivities();

    for (int i = 1; i < traceSize; i++)
    {
        if (rightActivities.count(trace->at(i)) &&
            leftActivities.count(trace->at(i - 1)))
        {
            splitPositions.push_back(i);
        }
    }

    for (const auto splitPosition : splitPositions)
    {
        segments.push_back({splitPosition, traceSize - splitPosition});
    }

    return segments;
}

// Helper struct for edge costs in Dijkstra implementation
// C++ requires explicit struct definition unlike Python's tuples
struct PairCost
{
    std::pair<int, int> first_pair;
    std::pair<int, int> second_pair;
    int cost;

    PairCost(std::pair<int, int> fp, std::pair<int, int> sp, int c)
        : first_pair(fp), second_pair(sp), cost(c) {}

    PairCost() : first_pair({-1, -1}), second_pair({-1, -1}), cost(-1) {}
};

// Generates outgoing edges for Dijkstra algorithm
// analogous to Python version
std::vector<PairCost> outgoingEdges(std::pair<int, int> v, std::shared_ptr<IntVec> trace, std::shared_ptr<TreeNode> node)
{
    int n = trace->size();
    auto &children = node->getChildren();
    int numChild = children.size();

    std::vector<PairCost> result;
    if (v.first == numChild)
    {
        return result;
    }
    for (int k = v.second; k < n + 1; k++)
    {
        if (v.first == numChild - 1 && k < n)
        {
            continue;
        }
        if (k < n - 1 && children.at(v.first)->getActivities().count(trace->at(k)))
        {
            continue;
        }
        auto subTrace = createSubtrace(trace, v.second, k);
        int tempCost = dynAlign(children.at(v.first), subTrace);
        result.push_back(PairCost(std::pair<int, int>(v.first, v.second), std::pair<int, int>(v.first + 1, k), tempCost));
    }
    return result;
}

// Implements _dyn_align_sequence from Python with C++ idioms
int dynAlignSequence(std::shared_ptr<TreeNode> node, const std::shared_ptr<IntVec> trace)
{
    int n = trace->size();
    const auto &children = node->getChildren();
    int numChildren = children.size();

    if (n == 0)
    {
        // C++ uses std::accumulate with lambda instead of Python's sum() with list comprehension
        return std::accumulate(children.begin(), children.end(), 0, [&trace](int sum, const auto &child)
                               { return sum + dynAlign(child, trace); });
    }

    // special case for binary sequence operator (common case optimization)
    if (numChildren == 2)
    {
        int costs = std::numeric_limits<int>::max();
        // remove elements that are not in the subtree
        // TODO this way the trace always has to be recomputed? maybe there could be a more efficient solution
        std::shared_ptr<IntVec> prunedTrace = pruneTrace(children, trace);
        int prunedN = prunedTrace->size();
        int aliens = n - prunedN;

        auto segments = getSegmentsForSequence(prunedTrace, node);
        for (const auto segment : segments)
        {
            int split = segment.first;
            auto firstPart = createSubtrace(prunedTrace, 0, split);
            auto secondPart = createSubtrace(prunedTrace, split, prunedN);

            auto firstLowerBound = estimateLowerBound(children[0], firstPart);
            if (firstLowerBound + aliens >= costs)
            {
                // std::cout << "first lower bound termination: " << firstLowerBound << " bigger than costs: " << costs << std::endl;
                continue;
            }
            auto secondLowerBound = estimateLowerBound(children[1], secondPart);
            if (firstLowerBound + secondLowerBound + aliens >= costs)
            {
                // std::cout << "second lower bound termination: " << secondLowerBound << " bigger than costs: " << costs << std::endl;
                continue;
            }

            auto leftCost = dynAlign(children[0], firstPart) + aliens;
            auto rightCost = dynAlign(children[1], secondPart);

            costs = std::min(leftCost + rightCost, costs);
        }
        return costs;
    }

    std::vector<std::pair<int, int>> vertices;

    for (int i = 0; i <= numChildren; ++i)
    {
        for (int j = 0; j <= n; ++j)
        {
            if ((i > 0 && i < numChildren) || (i == 0 && j == 0) || (i == numChildren && j == n))
            {
                vertices.push_back(std::make_pair(i, j));
            }
        }
    }

    std::pair<int, int> start(0, 0);
    std::pair<int, int> end(numChildren, n);

    // Using unordered_map with custom hash for pair keys (would be simple dict in Python)
    std::unordered_map<std::pair<int, int>, int, PairHash> dijkstraCosts;

    for (const auto vertex : vertices)
    {
        dijkstraCosts[vertex] = std::numeric_limits<int>::max();
    }
    dijkstraCosts[start] = 0;
    std::unordered_map<std::pair<int, int>, bool, PairHash> visited;

    // Dijkstra implementation - note Python would typically use a priority queue
    // right now not very efficient
    const auto verticesSize = vertices.size();
    while (visited.size() < verticesSize)
    {
        std::pair<int, int> current;
        int min_cost = std::numeric_limits<int>::max();

        for (const auto &v : vertices)
        {
            if (visited.find(v) == visited.end() && dijkstraCosts[v] <= min_cost)
            {
                min_cost = dijkstraCosts[v];
                current = v;
            }
        }
        visited[current] = true;

        for (auto edge : outgoingEdges(current, trace, node))
        {
            if (dijkstraCosts[current] != std::numeric_limits<int>::max())
            {
                dijkstraCosts[edge.second_pair] = std::min(dijkstraCosts[edge.second_pair], dijkstraCosts[current] + edge.cost);
            }
        }
    }

    return dijkstraCosts[end];
}

// Equivalent to Python's _dyn_align_shuffle
int dynAlignParallel(std::shared_ptr<TreeNode> node, const std::shared_ptr<IntVec> trace)
{
    const auto &children = node->getChildren();
    // Create vector of empty trace vectors for each child
    std::vector<std::shared_ptr<IntVec>> subTraces(children.size());
    std::generate(subTraces.begin(), subTraces.end(), []
                  { return std::make_shared<IntVec>(); });

    // Count unmatched activities and assign activities to child subtraces
    int unmatched = std::count_if(trace->begin(), trace->end(), [&](const int &activity)
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

// Equivalent to Python's _dyn_align_xor
int dynAlignXor(std::shared_ptr<TreeNode> node, const std::shared_ptr<IntVec> trace)
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

// Implements _dyn_align_loop from Python using C++ constructs
// for traces of form R(QR)*
int dynAlignLoop(std::shared_ptr<TreeNode> node, const std::shared_ptr<IntVec> trace)
{
    auto &children = node->getChildren();

    if (children.size() != 2)
    {
        throw std::runtime_error("Loop node with id: " + node->getId() + " does not have exactly two children.");
    }

    int n = trace->size();
    if (n == 0)
    {
        return dynAlign(children[0], trace);
    }

    std::vector<std::pair<int, int>> edges;
    for (int i = 0; i <= n; ++i)
    {
        for (int j = i; j <= n; ++j)
        {
            edges.emplace_back(i, j);
        }
    }

    // QR bits are aligned by introducing a temporary sequence node
    // and using alignSequence
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
        auto subTrace = createSubtrace(trace, edge.first, edge.second);
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
        rCosts[i] = dynAlign(children[0], createSubtrace(trace, 0, i));
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

// some event logs use a XorLoop instead of a RedoLoop
int dynAlignXorLoop(std::shared_ptr<TreeNode> node, const std::shared_ptr<IntVec> trace)
{
    // TODO temporary solution that fits data set redo(child[0], child[1])
    // this representation should be correct:     // sequence(child[0], redo(child[2], sequence(child[1], child[0])))
    auto &children = node->getChildren();

    auto tempRedoLoop = std::make_shared<TreeNode>(REDO_LOOP);
    tempRedoLoop->addChild(children[0]);
    tempRedoLoop->addChild(children[1]);

    int cost = dynAlignLoop(tempRedoLoop, trace);
    return cost;
}

// Activity node alignment - equivalent to _dyn_align_leaf in Python
// C++ needs to explicitly check if element exists in vector using std::find
int dynAlignActivity(std::shared_ptr<TreeNode> node, std::shared_ptr<IntVec> trace)
{
    const int &activity = node->getActivity();

    if (std::find(trace->begin(), trace->end(), activity) == trace->end())
    {
        return trace->size() + 1;
    }
    else
    {
        return trace->size() - 1;
    }
}

int dynAlignSilentActivity(std::shared_ptr<TreeNode> node, const std::shared_ptr<IntVec> trace)
{
    return trace->size();
}

int dynAlign(std::shared_ptr<TreeNode> node, const std::shared_ptr<IntVec> trace)
{
    std::string nodeId = node->getId();
    if (costTable.count(nodeId) > 0)
    {
        if (costTable[nodeId].count(*trace) == 1)
        {
            return costTable[nodeId][*trace];
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

    costTable[nodeId][*trace] = costs;
    return costs;
}
