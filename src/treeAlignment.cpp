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
#include <unordered_map>
#include <stack>

using StringVec = std::vector<std::string>;
using IntVec = std::vector<int>;
using IntPair = std::pair<int, int>;

// Forward declaration necessary in C++ (unlike Python where functions can be called before definition)
const size_t dynAlign(std::shared_ptr<TreeNode> node, const std::span<const int> trace);

std::vector<IntPair> outgoingEdges(const IntPair &vertex, std::shared_ptr<TreeNode> node, std::vector<size_t> &splitPositions)
{
    std::vector<IntPair> result;
    size_t const numChild = node->getChildren().size();

    if (vertex.first >= numChild - 1)
    {
        return result;
    }

    if (vertex.first == numChild - 2)
    {
        result.push_back({vertex.first + 1, splitPositions.back()});
        return result;
    }

    // use binary search to find start where elements are bigger than
    auto it = std::lower_bound(splitPositions.begin(), splitPositions.end(), vertex.second);

    // TODO: later use heuristic to sort this order
    for (; it != splitPositions.end(); ++it)
    {
        result.push_back({vertex.first + 1, *it});
    }

    return result;
}

// Implements _dyn_align_sequence from Python with C++ idioms
const int dynAlignSequence(const std::shared_ptr<TreeNode> node, const std::span<const int> trace)
{
    const auto &children = node->getChildren();
    const int childCount = children.size();
    const int traceLength = trace.size();

    if (traceLength == 0)
    {
        // C++ uses std::accumulate with lambda instead of Python's sum() with list comprehension
        return std::accumulate(children.begin(), children.end(), 0, [&trace](size_t sum, const auto &child)
                               { return sum + dynAlign(child, trace); });
    }
    
    std::unordered_map<int, size_t> activityToChildIndex;
    size_t childIndex = 0;
    for (const auto &child : children)
    {
        for (const auto activity : child->getActivities())
        {
            activityToChildIndex[activity] = childIndex;
        }
        ++childIndex;
    }

    std::vector<size_t> splitPositions = {0};
    for (size_t i = 1; i < traceLength; i++)
    {
        if (activityToChildIndex[trace[i]] != activityToChildIndex[trace[i - 1]])
        {
            splitPositions.push_back(i);
        }
    }
    splitPositions.push_back(traceLength);

    std::unordered_map<IntPair, size_t, PairHash> vertexCosts;
    for (size_t i = 0; i < childCount - 1; i++)
    {
        for (const auto splitPosition : splitPositions)
        {
            const IntPair vertex = {i, splitPosition};
            vertexCosts[vertex] = std::numeric_limits<size_t>::max();
        }
    }

    const IntPair finalVertex = {childCount - 1, splitPositions.back()};
    const IntPair startVertex = {-1, 0};
    vertexCosts[startVertex] = 0;
    vertexCosts[finalVertex] = std::numeric_limits<size_t>::max();

    std::stack<IntPair> stack;
    std::unordered_map<IntPair, IntPair, PairHash> prevVertices;

    for (const size_t splitPosition : splitPositions)
    {
        const IntPair initialVertex = {0, splitPosition};
        stack.push(initialVertex);
        prevVertices[initialVertex] = startVertex;
    }

    size_t bestCost = std::numeric_limits<size_t>::max();
    while (!stack.empty())
    {
        const IntPair currVertex = stack.top();
        stack.pop();
        const IntPair prevVertex = prevVertices[currVertex];

        size_t tempCost;
        if (prevVertex.first == -1)
        {
            tempCost = dynAlign(children[currVertex.first], trace.subspan(0, currVertex.second));
        }
        else
        {
            tempCost = dynAlign(children[currVertex.first], trace.subspan(prevVertex.second, currVertex.second - prevVertex.second));
        }

        const size_t newCost = tempCost + vertexCosts[prevVertex];
        if (newCost >= bestCost || newCost >= vertexCosts[currVertex])
        {
            continue;
        }
        vertexCosts[currVertex] = newCost;

        if (currVertex == finalVertex)
        {
            bestCost = newCost;
            continue;
        }

        for (const auto &nextEdge : outgoingEdges(currVertex, node, splitPositions))
        {
            prevVertices[nextEdge] = currVertex;
            stack.push(nextEdge);
        }
    }

    return bestCost;
}

// Equivalent to Python's _dyn_align_shuffle
const size_t dynAlignParallel(const std::shared_ptr<TreeNode> node, const std::span<const int> trace)
{
    const auto &children = node->getChildren();
    // Create vector of empty trace vectors for each child
    std::vector<IntVec> subTraces(children.size());
    std::generate(subTraces.begin(), subTraces.end(), []
                  { return IntVec(); });

    // Count unmatched activities and assign activities to child subtraces
    const int unmatched = std::count_if(trace.begin(), trace.end(), [&](const int &activity)
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
const size_t dynAlignXor(const std::shared_ptr<TreeNode> node, const std::span<const int> trace)
{
    size_t minCost = std::numeric_limits<size_t>::max();
    for (const auto &child : node->getChildren())
    {
        const size_t cost = dynAlign(child, trace);
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
const size_t dynAlignLoop(const std::shared_ptr<TreeNode> node, const std::span<const int> trace)
{
    const auto &children = node->getChildren();

    if (children.size() != 2)
    {
        throw std::runtime_error("Loop node with id: " + node->getId() + " does not have exactly two children.");
    }

    const size_t n = trace.size();
    if (n == 0)
    {
        return dynAlign(children[0], trace);
    }

    std::vector<IntPair> edges;
    for (size_t i = 0; i <= n; ++i)
    {
        for (size_t j = i; j <= n; ++j)
        {
            edges.emplace_back(i, j);
        }
    }

    // QR bits are aligned by introducing a temporary sequence node
    // and using alignSequence
    auto tempNode = std::make_shared<TreeNode>(SEQUENCE);
    tempNode->addChild(children[1]);
    tempNode->addChild(children[0]);

    std::unordered_map<IntPair, size_t, PairHash> qrCosts;
    for (const auto &edge : edges)
    {
        if (edge.first == edge.second)
        {
            qrCosts[edge] = 0;
            continue;
        }
        const auto subTrace = trace.subspan(edge.first, edge.second - edge.first);
        const size_t cost = dynAlign(tempNode, subTrace);
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

            size_t optimalCost = qrCosts[edge];
            for (size_t j = edge.first + 1; j < edge.second; j++)
            {
                const size_t newCost = qrCosts[{edge.first, j}] + qrCosts[{j, edge.second}];
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

    std::unordered_map<size_t, size_t> rCosts(n);
    for (size_t i = 0; i <= n; i++)
    {
        rCosts[i] = dynAlign(children[0], trace.subspan(0, i));
    }

    size_t minimalCosts = std::numeric_limits<size_t>::max();
    for (size_t i = 0; i <= n; i++)
    {
        const size_t costs = rCosts[i] + qrCosts[{i, n}];
        if (costs < minimalCosts)
        {
            minimalCosts = costs;
        }
    }

    return minimalCosts;
}

// some event logs use a XorLoop instead of a RedoLoop
const size_t dynAlignXorLoop(const std::shared_ptr<TreeNode> node, const std::span<const int> trace)
{
    // TODO temporary solution that fits data set redo(child[0], child[1])
    // this representation should be correct:     // sequence(child[0], redo(child[2], sequence(child[1], child[0])))
    const auto &children = node->getChildren();

    auto tempRedoLoop = std::make_shared<TreeNode>(REDO_LOOP);
    tempRedoLoop->addChild(children[0]);
    tempRedoLoop->addChild(children[1]);

    const size_t cost = dynAlignLoop(tempRedoLoop, trace);
    return cost;
}

// Activity node alignment - equivalent to _dyn_align_leaf in Python
// C++ needs to explicitly check if element exists in vector using std::find
const size_t dynAlignActivity(const std::shared_ptr<TreeNode> node, const std::span<const int> trace)
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

const size_t dynAlignSilentActivity(const std::shared_ptr<TreeNode> node, const std::span<const int> trace)
{
    return trace.size();
}

const size_t dynAlign(const std::shared_ptr<TreeNode> node, const std::span<const int> trace)
{
    const std::string nodeId = node->getId();
    if (costTable.count(nodeId) > 0)
    {
        // Use find() with span directly - no conversion to vector needed
        const auto it = costTable[nodeId].find(trace);
        if (it != costTable[nodeId].end())
        {
            return it->second;
        }
    }

    size_t costs;
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

    const std::vector<int> traceVector(trace.begin(), trace.end());
    costTable[nodeId][traceVector] = costs;
    return costs;
}
