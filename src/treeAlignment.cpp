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
#include <unordered_map>
#include <stack>
#include <optional>
#include <chrono>

using StringVec = std::vector<std::string>;
using IntVec = std::vector<int>;
using IntPair = std::pair<int, int>;

std::optional<std::span<const int>> a;
// std::unordered_map<int, size_t> traceCounter;

// std::vector<int> filter_range_copy(int min_val, int max_val, const std::vector<int> &vec)
// {
//     bool needs_filtering = false;
//     for (int x : vec)
//     {
//         if (x < min_val || x > max_val)
//         {
//             needs_filtering = true;
//             break;
//         }
//     }
//     if (!needs_filtering)
//     {
//         return vec;
//     }
//     std::vector<int> result;
//     for (int x : vec)
//     {
//         if (x >= min_val && x <= max_val)
//         {
//             result.push_back(x);
//         }
//     }
//     return result;
// }

// void increaseCounter(std::vector<int> &currentIndices)
// {
//     for (auto index : currentIndices)
//     {
//         traceCounter[index]++;
//     }
// }

// Forward declaration necessary in C++ (unlike Python where functions can be called before definition)
const size_t dynAlign(std::shared_ptr<TreeNode> node, const std::span<const int> trace);

// Helper function to get segments - analogous to get_segments_for_sequence in Python
const std::vector<IntPair> getSegments(const std::span<const int> trace, std::shared_ptr<TreeNode> node)
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
    // const auto leftActivities = children[0]->getActivities();
    const auto rightActivities = children[1]->getActivities();

    for (size_t i = 1; i < traceSize; i++)
    {
        if (rightActivities.count(trace[i]) &&
            !rightActivities.count(trace[i - 1]))
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
    result.reserve(std::distance(it, splitPositions.end()));

    // TODO: later use heuristic to sort this order
    for (; it != splitPositions.end(); ++it)
    {
        result.push_back({vertex.first + 1, *it});
    }

    return result;
}

// Implements _dyn_align_sequence from Python with C++ idioms
const size_t dynAlignSequence(const std::shared_ptr<TreeNode> node, const std::span<const int> trace)
{
    // auto executionStart = std::chrono::high_resolution_clock::now();

    const auto &children = node->getChildren();
    const int childCount = children.size();
    const int traceLength = trace.size();

    if (traceLength == 0)
    {
        // C++ uses std::accumulate with lambda instead of Python's sum() with list comprehension
        auto x = std::accumulate(children.begin(), children.end(), 0, [&trace](size_t sum, const auto &child)
                                 { return sum + dynAlign(child, trace); });
        // auto executionEnd = std::chrono::high_resolution_clock::now();
        // std::cout << node->getId() << " : " << std::chrono::duration_cast<std::chrono::nanoseconds>(executionEnd - executionStart).count() << std::endl;

        return x;
    }

    if (childCount == 1)
    {
        // std::cout << "dynAlignSequence for 1 child " << std::endl << " for trace: " << visualizeSpanTrace(trace) << std::endl;
        auto x = dynAlign(children[0], trace);
        // auto executionEnd = std::chrono::high_resolution_clock::now();
        // std::cout << node->getId() << " : " << std::chrono::duration_cast<std::chrono::nanoseconds>(executionEnd - executionStart).count() << std::endl;
        return x;
    }
    else if (childCount == 2)
    {
        // std::cout << "dynAlignSequence for 2 child " << std::endl << " for trace: " << visualizeSpanTrace(trace) << std::endl;
        size_t upperBound = std::numeric_limits<size_t>::max();

        const auto segments = getSegments(trace, node);
        for (const auto &[split, _] : segments)
        {
            const auto firstPart = trace.subspan(0, split);
            // auto lfilteredIndices = filter_range_copy(0, split, indices);
            const auto leftCost = dynAlign(children[0], firstPart);

            if (leftCost >= upperBound)
            {
                continue;
            }

            const auto secondPart = trace.subspan(split, traceLength - split);
            // auto rfilteredIndicies = filter_range_copy(split, traceLength - split, indices);
            const auto rightCost = dynAlign(children[1], secondPart);

            upperBound = std::min(leftCost + rightCost, upperBound);
        }

        // auto executionEnd = std::chrono::high_resolution_clock::now();
        // std::cout << node->getId() << " : " << std::chrono::duration_cast<std::chrono::nanoseconds>(executionEnd - executionStart).count() << std::endl;
        return upperBound;
    }

    // std::cout << "dynAlignSequence for 3 child " << std::endl << " for trace: " << visualizeSpanTrace(trace) << std::endl;

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
            // auto filterVec = filter_range_copy(0, currVertex.second, indices);
            // tempCost = dynAlign(children[currVertex.first], trace.subspan(0, currVertex.second), filterVec);
            tempCost = dynAlign(children[currVertex.first], trace.subspan(0, currVertex.second));
        }
        else
        {
            // auto filterVec = filter_range_copy(prevVertex.second, currVertex.second - prevVertex.second, indices);
            // tempCost = dynAlign(children[currVertex.first], trace.subspan(prevVertex.second, currVertex.second - prevVertex.second),
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

    // auto executionEnd = std::chrono::high_resolution_clock::now();
    // std::cout << node->getId() << " : " << std::chrono::duration_cast<std::chrono::nanoseconds>(executionEnd - executionStart).count() << std::endl;
    return bestCost;
}

// Equivalent to Python's _dyn_align_shuffle
const size_t dynAlignParallel(const std::shared_ptr<TreeNode> node, const std::span<const int> trace)
{
    // auto executionStart = std::chrono::high_resolution_clock::now();

    const auto &children = node->getChildren();
    // Create vector of empty trace vectors for each child
    std::vector<IntVec> subTraces(children.size());
    // std::vector<IntVec> subIndices(children.size());
    std::generate(subTraces.begin(), subTraces.end(), []
                  { return IntVec(); });
    // std::generate(subIndices.begin(), subIndices.end(), []
    //               { return IntVec(); });

    // Count unmatched activities and assign activities to child subtraces
    const int unmatched = std::count_if(trace.begin(), trace.end(), [&](const int &activity)
                                        {
        // Calculate the current index in the original trace
        int currentIndex = &activity - &(*trace.begin());
        
        for (size_t i = 0; i < children.size(); i++) {
            if (children[i]->getActivities().count(activity)) {
                subTraces[i].push_back(activity);
                // Add this line to push the index to subIndices
                // subIndices[i].push_back(currentIndex);
                
                return false;
            }
        }
        return true; });

    int cost = 0;
    for (size_t i = 0; i < children.size(); ++i)
    {
        cost += dynAlign(children[i], std::span<const int>(subTraces[i]));
    }

    // auto executionEnd = std::chrono::high_resolution_clock::now();
    // std::cout << node->getId() << " : " << std::chrono::duration_cast<std::chrono::nanoseconds>(executionEnd - executionStart).count() << std::endl;
    return cost + unmatched;
}

// Equivalent to Python's _dyn_align_xor
const size_t dynAlignXor(const std::shared_ptr<TreeNode> node, const std::span<const int> trace)
{
    // auto executionStart = std::chrono::high_resolution_clock::now();

    size_t minCost = std::numeric_limits<size_t>::max();
    for (const auto &child : node->getChildren())
    {
        const size_t cost = dynAlign(child, trace);
        if (cost == 0)
        {
            // auto executionEnd = std::chrono::high_resolution_clock::now();
            // std::cout << node->getId() << " : " << std::chrono::duration_cast<std::chrono::nanoseconds>(executionEnd - executionStart).count() << std::endl;
            return cost;
        }
        minCost = std::min(minCost, cost);
    }

    // auto executionEnd = std::chrono::high_resolution_clock::now();
    // std::cout << node->getId() << " : " << std::chrono::duration_cast<std::chrono::nanoseconds>(executionEnd - executionStart).count() << std::endl;
    return minCost;
}

// Implements _dyn_align_loop from Python using C++ constructs
// for traces of form R(QR)*
const size_t dynAlignLoop(const std::shared_ptr<TreeNode> node, const std::span<const int> trace)
{
    // auto executionStart = std::chrono::high_resolution_clock::now();

    const auto &children = node->getChildren();

    // TODO temp outcomment because if testing
    // if (children.size() != 2)
    // {
    //     throw std::runtime_error("Loop node with id: " + node->getId() + " does not have exactly two children.");
    // }

    const size_t n = trace.size();
    if (n == 0)
    {
        // auto executionEnd = std::chrono::high_resolution_clock::now();
        // std::cout << node->getId() << " : " << std::chrono::duration_cast<std::chrono::nanoseconds>(executionEnd - executionStart).count() << std::endl;
        auto x = dynAlign(children[0], trace);
        return x;
    }

    // try brute force first
    size_t upperBound = std::numeric_limits<size_t>::max();

    // const auto &rChildrenActv = children[0]->getActivities();
    // const auto firstTraceVal = trace[0];
    // const auto lastTraceVal = trace[n - 1];

    // if (rChildrenActv.count(firstTraceVal) && rChildrenActv.count(lastTraceVal))
    // {
    //     std::vector<std::span<const int>> rParts;
    //     std::vector<std::span<const int>> qParts;

    //     size_t i = 0;
    //     while (i < n && rChildrenActv.count(trace[i]))
    //     {
    //         i += 1;
    //     }
    //     const auto startPart = trace.subspan(0, i);
    //     rParts.push_back(startPart);

    //     while (i < n)
    //     {
    //         size_t j = i;
    //         while (j < n && !(rChildrenActv.count(trace[j])))
    //         {
    //             j += 1;
    //         }
    //         const auto qPart = trace.subspan(i, j - i);
    //         qParts.push_back(qPart);

    //         i = j;
    //         while (i < n && rChildrenActv.count(trace[i]))
    //         {
    //             i += 1;
    //         }
    //         const auto rPart = trace.subspan(j, i - j);
    //         rParts.push_back(rPart);
    //     }

    //     upperBound = 0;

    //     for (size_t i = 0; i < rParts.size(); i++)
    //     {
    //         upperBound += dynAlign(children[0], rParts[i]);
    //     }

    //     for (size_t i = 0; i < qParts.size(); i++)
    //     {
    //         upperBound += dynAlign(children[1], qParts[i]);
    //     }
    // }

    // if (upperBound == 0)
    // {
    //     return 0;
    // }

    // QR bits are aligned by introducing a temporary sequence node
    // and using alignSequence
    auto tempNode = std::make_shared<TreeNode>(SEQUENCE, node->getId() + "_temp");
    tempNode->addChild(children[1]);
    tempNode->addChild(children[0]);

    std::unordered_map<IntPair, int, PairHash> qrCosts;

    std::stack<IntPair> stack;
    for (size_t i = 0; i <= n; i++)
    {
        // auto fitlerVec = filter_range_copy(0, i);
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
                // auto filterVec = filter_range_copy(edge.first, edge.second - edge.first, indices);
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
    // auto executionEnd = std::chrono::high_resolution_clock::now();
    // std::cout << node->getId() << " : " << std::chrono::duration_cast<std::chrono::nanoseconds>(executionEnd - executionStart).count() << std::endl;
    return upperBound;
}

// some event logs use a XorLoop instead of a RedoLoop
const size_t dynAlignXorLoop(const std::shared_ptr<TreeNode> node, const std::span<const int> trace)
{
    // TODO temporary solution that fits data set redo(child[0], child[1])
    // this representation should be correct:     // sequence(child[0], redo(child[2], sequence(child[1], child[0])))
    // const auto &children = node->getChildren();

    // auto tempRedoLoop = std::make_shared<TreeNode>(REDO_LOOP);
    // tempRedoLoop->addChild(children[0]);
    // tempRedoLoop->addChild(children[1]);

    // const size_t cost = dynAlignLoop(tempRedoLoop, trace);
    return dynAlignLoop(node, trace);
}

// Activity node alignment - equivalent to _dyn_align_leaf in Python
// C++ needs to explicitly check if element exists in vector using std::find
const size_t dynAlignActivity(const std::shared_ptr<TreeNode> node, const std::span<const int> trace)
{
    // std::cout << "dynAlignActivity" << std::endl << " for trace: " << visualizeSpanTrace(trace) << std::endl;
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
    // std::cout << "dynSilentAlignActivity" << std::endl << " for trace: " << visualizeSpanTrace(trace) << std::endl;
    return trace.size();
}

const size_t dynAlign(const std::shared_ptr<TreeNode> node, std::span<const int> trace)
{
    // increaseCounter(indices);
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
    
    std::cout << visualizeSpanTrace(trace) << std::endl;

    auto &activities = node->getActivities();
    std::vector<int> prunedTrace; 
    for (const auto x : trace)
    {
        if (activities.count(x) == 0)
        {
            for (const int val : trace) {
                if (activities.count(val) != 0) {
                    prunedTrace.push_back(val);
                }
            }
            trace = std::span(prunedTrace);;
            break;
        }
    }
    std::cout << visualizeSpanTrace(trace) << std::endl;
    // float aliens = 0;
    // // Cast 'aliens' to float before division
    // float percent = static_cast<float>(aliens) / trace.size();
    // std::cout << percent << std::endl;
    // std::cout << compareSubsequenceStart(trace) << std::endl;
    // if (node->getOperation() != XOR_LOOP) {
    //     std::cout << node->getId() << std::endl;
    // }

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
