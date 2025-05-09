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
#include <unordered_map>

using IntVec = std::vector<int>;
using IntPair = std::pair<int, int>;

// Forward declaration necessary in C++ (unlike Python where functions can be called before definition)
const size_t dynAlign(std::shared_ptr<TreeNode> node, const std::span<const int> trace);

// Helper function to get segments - analogous to get_segments_for_sequence in Python
const std::vector<IntPair> getSegmentsForSequence(const std::span<const int> trace, std::shared_ptr<TreeNode> node)
{
    const auto &children = node->getChildren();
    if (children.size() != 2)
    {
        throw std::runtime_error("get_segments_for_sequence not implemented for more/less than two children.");
    }

    const size_t traceSize = trace.size();
    std::vector<IntPair> segments = {
        {0, traceSize},
        {traceSize, 0}};

    IntVec splitPositions;
    const auto leftActivities = children[0]->getActivities();
    const auto rightActivities = children[1]->getActivities();

    for (size_t i = 1; i < traceSize; i++)
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
const std::vector<PairCost> outgoingEdges(const IntPair v, const std::span<const int> trace, std::shared_ptr<TreeNode> node, size_t upperBound)
{
    const size_t n = trace.size();
    const auto &children = node->getChildren();
    const size_t numChildren = children.size();

    std::vector<PairCost> result;
    if (v.first == numChildren)
    {
        return result;
    }
    for (int k = v.second; k < n + 1; k++)
    {
        if (v.first == numChildren - 1 && k < n)
        {
            continue;
        }
        if (k < n - 1 && children.at(v.first)->getActivities().count(trace[k]))
        {
            continue;
        }
        const auto subTrace = trace.subspan(v.second, k - v.second);
        const int tempCost = dynAlign(children.at(v.first), subTrace);
        if (tempCost > upperBound)
        {
            continue;
        }
        result.push_back(PairCost(IntPair(v.first, v.second), IntPair(v.first + 1, k), tempCost));
    }
    return result;
}

std::vector<IntPair> outgoingEdges(const IntPair &vertex, std::shared_ptr<TreeNode> node, std::vector<size_t> &splitPositions)
{
    std::vector<IntPair> result;
    size_t const numChildren = node->getChildren().size();

    if (vertex.first >= numChildren - 1)
    {
        return result;
    }

    if (vertex.first == numChildren - 2)
    {
        result.push_back({vertex.first + 1, splitPositions.back()});
        return result;
    }

    // use binary search to find start where elements are bigger than
    auto it = std::lower_bound(splitPositions.begin(), splitPositions.end(), vertex.second);
    result.reserve(std::distance(it, splitPositions.end()));

    for (; it != splitPositions.end(); ++it)
    {
        result.push_back({vertex.first + 1, *it});
    }

    return result;
}

// has an upper bound estimation
const std::vector<PairCost> outgoingEdges(const IntPair v, const std::span<const int> trace, std::shared_ptr<TreeNode> node)
{
    const size_t n = trace.size();
    const auto &children = node->getChildren();
    const size_t numChildren = children.size();

    std::vector<PairCost> result;
    if (v.first == numChildren)
    {
        return result;
    }
    for (int k = v.second; k < n + 1; k++)
    {
        if (v.first == numChildren - 1 && k < n)
        {
            continue;
        }
        if (k < n - 1 && children.at(v.first)->getActivities().count(trace[k]))
        {
            continue;
        }
        const auto subTrace = trace.subspan(v.second, k - v.second);
        const int tempCost = dynAlign(children.at(v.first), subTrace);
        result.push_back(PairCost(IntPair(v.first, v.second), IntPair(v.first + 1, k), tempCost));
    }
    return result;
}

// Implements _dyn_align_sequence from Python with C++ idioms
const size_t dynAlignSequence(const std::shared_ptr<TreeNode> node, const std::span<const int> trace)
{

    const auto &children = node->getChildren();
    const int numChildren = children.size();
    const int traceLength = trace.size();
    size_t bestCost = std::numeric_limits<size_t>::max(); // modify later so that it is the same for both versions

    if (traceLength == 0)
    {
        // C++ uses std::accumulate with lambda instead of Python's sum() with list comprehension
        return std::accumulate(children.begin(), children.end(), 0, [&trace](size_t sum, const auto &child)
                               { return sum + dynAlign(child, trace); });
    }

    if (numChildren == 1)
    {
        return dynAlign(children[0], trace);
    }

    // TODO has to be pushed up a bit.
#if ENABLE_UPPER_BOUND == 1
    // std::cout << visualizeSpanTrace(trace) << std::endl;
    // std::cout << "0" << std::endl;
    // std::cout << children.size() << std::endl;
    // std::cout << node->getId() << std::endl;

    size_t pos = 0;
    size_t old_pos = 0;
    bestCost = 0;

    // Try greedy approach first - attempt to partition trace by activity membership
    if (traceLength > numChildren &&
        children[0]->getActivities().count(trace[0]) &&
        children.back()->getActivities().count(trace.back()))
    {
        for (const auto &child : children)
        {
            while (pos < trace.size() && child->getActivities().count(trace[pos]))
            {
                pos += 1;
            }
            const auto subTrace = trace.subspan(old_pos, pos - old_pos);
            bestCost += dynAlign(child, subTrace);
            old_pos = pos;
        }
    }

    if (pos < trace.size())
    {
        bestCost = std::numeric_limits<size_t>::max();
    }

#endif

    if (numChildren == 2)
    {
        // std::cout << "1" << std::endl;

        const auto segments = getSegmentsForSequence(trace, node);
        for (const auto &[split, _] : segments)
        {
            const auto firstPart = trace.subspan(0, split);
            const auto leftCost = dynAlign(children[0], firstPart);

#if ENABLE_UPPER_BOUND == 1

            if (leftCost >= bestCost)
            {
                continue;
            }
#endif
            const auto secondPart = trace.subspan(split, traceLength - split);
            const auto rightCost = dynAlign(children[1], secondPart);

            // TODO check if 0 and then early exit
            bestCost = std::min(leftCost + rightCost, bestCost);
        }
        return bestCost;
    }

#if SEQUENCE_IMPROVEMENT == 1

    // std::cout << "2" << std::endl;

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
    for (size_t i = 0; i < numChildren - 1; i++)
    {
        for (const auto splitPosition : splitPositions)
        {
            const IntPair vertex = {i, splitPosition};
            vertexCosts[vertex] = std::numeric_limits<size_t>::max();
        }
    }

    const IntPair finalVertex = {numChildren - 1, splitPositions.back()};
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

#else

    // std::cout << "3" << std::endl;

    std::vector<IntPair> vertices;

    for (size_t i = 0; i <= numChildren; ++i)
    {
        for (size_t j = 0; j <= traceLength; ++j)
        {
            if ((i > 0 && i < numChildren) || (i == 0 && j == 0) || (i == numChildren && j == traceLength))
            {
                vertices.push_back(std::make_pair(i, j));
            }
        }
    }

    IntPair start(0, 0);
    IntPair end(numChildren, traceLength);

    std::unordered_map<IntPair, size_t, PairHash> dijkstraCosts;

    for (const auto vertex : vertices)
    {
        dijkstraCosts[vertex] = std::numeric_limits<size_t>::max();
    }
    dijkstraCosts[start] = 0;
    std::unordered_set<IntPair, PairHash> visited;

    const auto verticesSize = vertices.size();
    while (visited.size() < verticesSize)
    {
        IntPair current;
        size_t min_cost = std::numeric_limits<size_t>::max();

        for (const auto &v : vertices)
        {
            if (visited.find(v) == visited.end() && dijkstraCosts[v] <= min_cost)
            {
                min_cost = dijkstraCosts[v];
                current = v;
            }
        }
        visited.insert(current);
#if ENABLE_UPPER_BOUND == 1
        if (dijkstraCosts[current] > bestCost)
        {
            continue;
        }
        for (const auto edge : outgoingEdges(current, trace, node, bestCost))
#else
        for (const auto edge : outgoingEdges(current, trace, node))
#endif
        {
            if (dijkstraCosts[current] != std::numeric_limits<size_t>::max())
            {
                dijkstraCosts[edge.second_pair] = std::min(dijkstraCosts[edge.second_pair], dijkstraCosts[current] + edge.cost);
            }
        }
    }

    return dijkstraCosts[end];
#endif
}

// Equivalent to Python's _dyn_align_shuffle
const size_t dynAlignParallel(const std::shared_ptr<TreeNode> node, const std::span<const int> trace)
{
    // std::cout << "parallel" << std::endl;

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

// for traces of form R(QR)*
const size_t dynAlignLoop(const std::shared_ptr<TreeNode> node, const std::span<const int> trace)
{
    // std::cout << "looop" << std::endl;
    const auto &children = node->getChildren();

    if (children.size() != 2)
    {
        throw std::runtime_error("Loop node with id: " + std::to_string(node->getId()) + " does not have exactly two children.");
    }

    const size_t n = trace.size();
    if (n == 0)
    {
        return dynAlign(children[0], trace);
    }
    // TODO upperbound is not used yet
    size_t upperBound = std::numeric_limits<size_t>::max();

#if ENABLE_UPPER_BOUND == 1
    const auto &rChildrenActv = children[0]->getActivities();
    const auto firstTraceVal = trace[0];
    const auto lastTraceVal = trace[n - 1];

    if (rChildrenActv.count(firstTraceVal) && rChildrenActv.count(lastTraceVal))
    {
        std::vector<std::span<const int>> rParts;
        std::vector<std::span<const int>> qParts;

        size_t i = 0;
        while (i < n && rChildrenActv.count(trace[i]))
        {
            i += 1;
        }
        const auto startPart = trace.subspan(0, i);
        rParts.push_back(startPart);

        while (i < n)
        {
            size_t j = i;
            while (j < n && !(rChildrenActv.count(trace[j])))
            {
                j += 1;
            }
            const auto qPart = trace.subspan(i, j - i);
            qParts.push_back(qPart);

            i = j;
            while (i < n && rChildrenActv.count(trace[i]))
            {
                i += 1;
            }
            const auto rPart = trace.subspan(j, i - j);
            rParts.push_back(rPart);
        }

        upperBound = 0;

        for (size_t i = 0; i < rParts.size(); i++)
        {
            upperBound += dynAlign(children[0], rParts[i]);
        }

        for (size_t i = 0; i < qParts.size(); i++)
        {
            upperBound += dynAlign(children[1], qParts[i]);
        }
    }

    if (upperBound == 0)
    {
        return 0;
    }
#endif

    // QR bits are aligned by introducing a temporary sequence node
    // and using alignSequence

#if TEMP_SEQUENCE_STORING == 1
    const int tempNodeId = node->getId() * -1;
    std::shared_ptr<TreeNode> tempNode;

    auto it = tempNodeMap.find(tempNodeId);
    if (it != tempNodeMap.end())
    {
        tempNode = it->second;
    }
    else
    {
        std::cerr << "Didn't find tempnode for node with id: " << std::to_string(static_cast<int>(node->getId())) << std::endl;
        tempNode = std::make_shared<TreeNode>(SEQUENCE, tempNodeId);
        tempNodeMap[tempNodeId] = tempNode;
#if TRACE_PRUNING == 1
        std::unordered_set<int> allActivities;
        allActivities.reserve(children[0]->getActivities().size() + children[1]->getActivities().size());

        std::set_union(
            children[0]->getActivities().begin(), children[0]->getActivities().end(),
            children[1]->getActivities().begin(), children[1]->getActivities().end(),
            std::inserter(allActivities, allActivities.begin()));

        tempNode->setActivities(allActivities);
#endif
    }
#else
    std::shared_ptr<TreeNode> tempNode = std::make_shared<TreeNode>(SEQUENCE);

#if TRACE_PRUNING == 1
    std::unordered_set<int> allActivities;
    allActivities.reserve(children[0]->getActivities().size() + children[1]->getActivities().size());

    std::set_union(
        children[0]->getActivities().begin(), children[0]->getActivities().end(),
        children[1]->getActivities().begin(), children[1]->getActivities().end(),
        std::inserter(allActivities, allActivities.begin()));

    tempNode->setActivities(allActivities);
#endif
    tempNode->addChild(children[1]);
    tempNode->addChild(children[0]);
#endif

#if DFS_LOOP == 1
    std::unordered_map<IntPair, int, PairHash>
        qrCosts;

    std::stack<IntPair> stack;
    for (size_t i = 0; i <= n; i++)
    {
        const size_t rCost = dynAlign(children[0], trace.subspan(0, i));

        stack.push(IntPair(i, i));
        bool firstStackElement = true;

        while (!stack.empty())
        {
            IntPair edge = stack.top();
            stack.pop();

            IntPair startEdge(0, edge.first);
            IntPair totalEdge(0, edge.second);

            size_t prevEdgesCost;
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

            size_t edgesCost;
            if (edge.second == edge.first)
            {
                // Empty segment case
                edgesCost = prevEdgesCost;
            }
            else
            {
                // Calculate alignment cost for this segment
                edgesCost = prevEdgesCost + dynAlign(
                                                tempNode,
                                                trace.subspan(edge.first, edge.second - edge.first));
            }

            if (edgesCost >= upperBound)
            {
                continue;
            }

            const auto existingEdge = qrCosts.find(totalEdge);
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
                for (size_t j = totalEdge.second + 1; j <= n; j++)
                {
                    IntPair newEdge(totalEdge.second, j);
                    stack.push(newEdge);
                }
            }
        }
    }
    return upperBound;

#else

    std::vector<IntPair> edges;
    for (size_t i = 0; i <= n; ++i)
    {
        for (size_t j = i; j <= n; ++j)
        {
            edges.emplace_back(i, j);
        }
    }

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
#endif
}

// Activity node alignment - equivalent to _dyn_align_leaf in Python
// C++ needs to explicitly check if element exists in vector using std::find
const size_t dynAlignActivity(const std::shared_ptr<TreeNode> node, const std::span<const int> trace)
{
    // std::cout << "activity" << std::endl;

    const int activity = node->getId();

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

const size_t dynAlign(const std::shared_ptr<TreeNode> node, std::span<const int> trace)
{
    // std::cout << "dynAlign" << std::endl;

    const int nodeId = node->getId();

    auto [mapIt, wasInserted] = costTable.try_emplace(nodeId);
    auto &innerMap = mapIt->second;

    const auto it = innerMap.find(trace);
    if (it != innerMap.end())
    {
        return it->second;
    }

#if TRACE_PRUNING == 1
    size_t aliens = 0;
    auto &activities = node->getActivities();
    std::vector<int> prunedTrace;
    for (const auto x : trace)
    {
        if (activities.count(x) == 0)
        {
            for (const int val : trace)
            {
                if (activities.count(val) != 0)
                {
                    prunedTrace.push_back(val);
                }
                else
                {
                    aliens++;
                }
            }
            trace = std::span(prunedTrace);
            break;
        }
    }
#endif

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
    case ACTIVITY:
        costs = dynAlignActivity(node, trace);
        break;
    case SILENT_ACTIVITY:
        costs = dynAlignSilentActivity(node, trace);
        break;
    default:
        throw std::runtime_error("Unknown node operation: " + std::to_string(node->getOperation()));
    }

#if TRACE_PRUNING
    if (!prunedTrace.empty())
    {
        costTable[nodeId][std::move(prunedTrace)] = costs;
    }
    else
    {
        const std::vector<int> traceVector(trace.begin(), trace.end());
        costTable[nodeId][std::move(traceVector)] = costs;
    }
    return costs + aliens;
#else
    const std::vector<int> traceVector(trace.begin(), trace.end());
    costTable[nodeId][std::move(traceVector)] = costs;
    return costs;
#endif
}
