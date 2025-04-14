#include "treeNode.h"
#include "utils.h"
#include "parser.h"
#include <memory>
#include <string>
#include <numeric>
#include <limits>
#include <stack>
#include <vector>
#include <algorithm>
#include <iostream>
#include <cmath>

using StringVec = std::vector<std::string>;
using IntVec = std::vector<int>;
using IntPair = std::pair<int, int>;

// Forward declaration necessary in C++ (unlike Python where functions can be called before definition)
int dynAlign(std::shared_ptr<TreeNode> node, std::span<const int> trace);

// Helper function to get segments - analogous to get_segments_for_sequence in Python
std::vector<IntPair> getSegmentsForSequence(std::span<const int> trace, std::shared_ptr<TreeNode> node)
{
    auto &children = node->getChildren();
    if (children.size() != 2)
    {
        throw std::runtime_error("get_segments_for_sequence not implemented for more/less than two children.");
    }

    int traceSize = trace.size();
    std::vector<IntPair> segments = {
        {0, traceSize},
        {traceSize, 0}};

    IntVec splitPositions;
    auto leftActivities = children[0]->getActivities();
    auto rightActivities = children[1]->getActivities();

    for (int i = 1; i < traceSize; i++)
    {
        if (rightActivities.count(trace[i]) &&
            leftActivities.count(trace[i - 1]))
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
    IntPair first_pair;
    IntPair second_pair;
    int cost;

    PairCost(IntPair fp, IntPair sp, int c)
        : first_pair(fp), second_pair(sp), cost(c) {}

    PairCost() : first_pair({-1, -1}), second_pair({-1, -1}), cost(-1) {}
};

// Generates outgoing edges for Dijkstra algorithm
// analogous to Python version
std::vector<PairCost> outgoingEdges(IntPair v, std::span<const int> trace, std::shared_ptr<TreeNode> node)
{
    int n = trace.size();
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
        if (k < n - 1 && children.at(v.first)->getActivities().count(trace[k]))
        {
            continue;
        }
        auto subTrace = trace.subspan(v.second, k - v.second);
        int tempCost = dynAlign(children.at(v.first), subTrace);
        result.push_back(PairCost(IntPair(v.first, v.second), IntPair(v.first + 1, k), tempCost));
    }
    return result;
}

// Implements _dyn_align_sequence from Python with C++ idioms
int dynAlignSequence(std::shared_ptr<TreeNode> node, std::span<const int> trace)
{
    int n = trace.size();
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
        auto prunedTraceSpan = std::span<const int>(*prunedTrace);

        int prunedN = prunedTraceSpan.size();
        int aliens = n - prunedN;

        auto segments = getSegmentsForSequence(prunedTraceSpan, node);
        for (const auto segment : segments)
        {
            int split = segment.first;
            auto firstPart = prunedTraceSpan.subspan(0, split);
            auto secondPart = prunedTraceSpan.subspan(split, prunedN - split);

            auto leftCost = dynAlign(children[0], firstPart) + aliens;
            auto rightCost = dynAlign(children[1], secondPart);

            costs = std::min(leftCost + rightCost, costs);
        }
        return costs;
    }

    std::vector<IntPair> vertices;

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

    IntPair start(0, 0);
    IntPair end(numChildren, n);

    // Using unordered_map with custom hash for pair keys (would be simple dict in Python)
    std::unordered_map<IntPair, int, PairHash> dijkstraCosts;

    for (const auto vertex : vertices)
    {
        dijkstraCosts[vertex] = std::numeric_limits<int>::max();
    }
    dijkstraCosts[start] = 0;
    std::unordered_map<IntPair, bool, PairHash> visited;

    // Dijkstra implementation - note Python would typically use a priority queue
    // right now not very efficient
    const auto verticesSize = vertices.size();
    while (visited.size() < verticesSize)
    {
        IntPair current;
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
int dynAlignParallel(std::shared_ptr<TreeNode> node, std::span<const int> trace)
{
    const auto &children = node->getChildren();
    // Create vector of empty trace vectors for each child
    std::vector<IntVec> subTraces(children.size());
    std::generate(subTraces.begin(), subTraces.end(), []
                  { return IntVec(); });

    // Count unmatched activities and assign activities to child subtraces
    int unmatched = std::count_if(trace.begin(), trace.end(), [&](const int &activity)
                                  {
            for (size_t i = 0; i < children.size(); i++) {
                if (children[i]->getActivities().count(activity)) {
                    subTraces[i].push_back(activity);
                    return false;
                }
            }
        return true; });

    int cost = 0;
    for (size_t i = 0; i < children.size(); ++i)
    {
        cost += dynAlign(children[i], std::span<const int>(subTraces[i]));
    }

    return cost + unmatched;
}

// Equivalent to Python's _dyn_align_xor
int dynAlignXor(std::shared_ptr<TreeNode> node, std::span<const int> trace)
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

// returns a ranking measure based on how favorable this edge is
// update this to work more as a lower bound
float estimateEdgeCost(IntPair edge, std::span<const int> trace, std::shared_ptr<TreeNode> node)
{
    const auto& activities = node->getActivities();
    int commonActivities = 0;

    for (int i = edge.first; i < edge.second; i++)
    {
        if (activities.count(trace[i]))
        {
            commonActivities++;
        }
    }

    return std::pow(commonActivities, 2) / (edge.second - edge.first);
}

// Implements _dyn_align_loop from Python using C++ constructs
// for traces of form R(QR)*
int dynAlignLoop(std::shared_ptr<TreeNode> node, std::span<const int> trace)
{
    const auto &children = node->getChildren();
    if (children.size() != 2)
    {
        throw std::runtime_error("Loop node with id: " + node->getId() + " does not have exactly two children.");
    }

    const int n = trace.size();
    if (n == 0)
    {
        return dynAlign(children[0], trace);
    }

    // QR bits are aligned by introducing a temporary sequence node
    // and using alignSequence
    auto tempNode = std::make_shared<TreeNode>(SEQUENCE);
    tempNode->addChild(children[1]);
    tempNode->addChild(children[0]);

    std::unordered_map<IntPair, int, PairHash> qrCosts;
    int upperBound = std::numeric_limits<int>::max();
    
    std::stack<IntPair> stack;
    for (size_t i = 0; i <= n; i++)
    {
        int rCost = dynAlign(children[0], trace.subspan(0, i));

        stack.push(IntPair(i, i));
        bool firstStackElement = true;

        while (!stack.empty())
        {
            IntPair edge = stack.top();
            stack.pop();

            IntPair startEdge(0, edge.first);
            IntPair totalEdge(0, edge.second);

            int prevEdgesCost;
            if (firstStackElement)
            {
                prevEdgesCost = rCost;
                firstStackElement = false;
            }
            else
            {
                prevEdgesCost = qrCosts[startEdge];
            }

            if (prevEdgesCost >= upperBound)
            {
                continue;
            }

            int edgesCost;
            if (edge.second == edge.first) {
                // Empty segment case
                edgesCost = prevEdgesCost;
            } else {
                // Calculate alignment cost for this segment
                edgesCost = prevEdgesCost + dynAlign(
                    tempNode, 
                    trace.subspan(edge.first, edge.second - edge.first)
                );
            }

            if (edgesCost >= upperBound)
            {
                continue;
            }

            auto existingEdge = qrCosts.find(totalEdge);
            if (existingEdge != qrCosts.end() && edgesCost >= existingEdge->second)
            {
                continue;
            }

            qrCosts[totalEdge] = edgesCost;

            if (totalEdge.second == n)
            {
                upperBound = edgesCost;
            }
            else
            {
                // calculate outgoing edges
                float bestHeuristic = -1.0;
                IntPair bestEdge;
                for (int i = totalEdge.second + 1; i <= n; i++)
                {
                    IntPair newEdge(totalEdge.second, i);
                    float estimate = estimateEdgeCost(newEdge, trace, tempNode);
                    // perhaps sort them, not only get the best one??
                    if (estimate > bestHeuristic)
                    {
                        if (bestHeuristic != -1.0)
                        {
                            stack.push(bestEdge);
                        }
                        bestHeuristic = estimate;
                        bestEdge = newEdge;
                    }
                    else
                    {
                        stack.push(newEdge);
                    }
                }
                stack.push(bestEdge);
            }
        }
    }
    return upperBound;
}

// some event logs use a XorLoop instead of a RedoLoop
int dynAlignXorLoop(std::shared_ptr<TreeNode> node, std::span<const int> trace)
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
int dynAlignActivity(std::shared_ptr<TreeNode> node, std::span<const int> trace)
{
    const int &activity = node->getActivity();

    if (std::find(trace.begin(), trace.end(), activity) == trace.end())
    {
        return trace.size() + 1;
    }
    else
    {
        return trace.size() - 1;
    }
}

int dynAlignSilentActivity(std::shared_ptr<TreeNode> node, std::span<const int> trace)
{
    return trace.size();
}

int dynAlign(std::shared_ptr<TreeNode> node, std::span<const int> trace)
{
    std::string nodeId = node->getId();
    if (costTable.count(nodeId) > 0)
    {
        // Use find() with span directly - no conversion to vector needed
        auto it = costTable[nodeId].find(trace);
        if (it != costTable[nodeId].end())
        {
            return it->second;
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

    std::vector<int> traceVector(trace.begin(), trace.end());
    costTable[nodeId][traceVector] = costs;
    return costs;
}
