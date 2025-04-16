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

template <typename T1, typename T2>
std::ostream &operator<<(std::ostream &os, const std::pair<T1, T2> &p)
{
    os << "(" << p.first << ", " << p.second << ")";
    return os;
}

// Forward declaration necessary in C++ (unlike Python where functions can be called before definition)
int dynAlign(std::shared_ptr<TreeNode> node, std::span<const int> trace);

// Implements _dyn_align_sequence from Python with C++ idioms
std::vector<IntPair> outgoingEdges(IntPair &vertex, std::shared_ptr<TreeNode> node, IntVec &splitPositions)
{
    // std::cout << "outgoing edges" << std::endl;
    std::vector<IntPair> result;
    int numChild = node->getChildren().size();

    if (vertex.first >= numChild - 1)
    {
        return result;
    }

    if (vertex.first == numChild - 2)
    {
        result.push_back({vertex.first + 1, splitPositions.back()});
        return result;
    }

    // TODO: later use heuristic to sort this order
    for (const int val : splitPositions)
    {
        if (val >= vertex.second)
        {
            result.push_back({vertex.first + 1, val});
        }
    }

    return result;
}

int dynAlignSequence(std::shared_ptr<TreeNode> node, std::span<const int> trace)
{
    // std::cout << "align sequence" << std::endl;
    // std::cout << "num children: " << node->getChildren().size() << std::endl;
    // std::cout << "lenght of trace: " << trace.size() << std::endl;
    const auto &children = node->getChildren();
    const int childCount = children.size();
    const int traceLength = trace.size();

    // std::cout << "activity to child" << std::endl;
    std::unordered_map<int, int> activityToChildIndex;
    int childIndex = 0;
    for (const auto &child : children)
    {
        for (const auto &[activity, _] : child->getActivities())
        {
            activityToChildIndex[activity] = childIndex;
        }
        ++childIndex;
    }

    // std::cout << "splitpositions" << std::endl;
    std::vector<int> splitPositions = {0};
    for (int i = 1; i < traceLength; i++)
    {
        if (activityToChildIndex[trace[i]] != activityToChildIndex[trace[i - 1]])
        {
            splitPositions.push_back(i);
        }
    }
    splitPositions.push_back(traceLength);

    // std::vector<IntPair> vertices;

    // std::cout << "vertices" << std::endl;
    std::unordered_map<IntPair, int, PairHash> vertexCosts;
    for (int i = 0; i < childCount - 1; i++)
    {
        for (const auto splitPosition : splitPositions)
        {
            IntPair vertex = {i, splitPosition};
            // vertices.push_back(vertex);
            vertexCosts[vertex] = std::numeric_limits<int>::max();
        }
    }

    const IntPair finalVertex = {childCount - 1, splitPositions.back()};
    const IntPair startVertex = {-1, 0};
    vertexCosts[startVertex] = 0;
    vertexCosts[finalVertex] = std::numeric_limits<int>::max();
    // vertices.push_back(finalVertex);

    std::stack<IntPair> stack;
    std::unordered_map<IntPair, IntPair, PairHash> predecessors;

    // std::cout << "init predecessors" << std::endl;
    for (const int splitPosition : splitPositions)
    {
        IntPair initialVertex = {0, splitPosition};
        stack.push(initialVertex);
        predecessors[initialVertex] = startVertex;
    }

    // std::cout << "dfs" << std::endl;
    int bestCost = std::numeric_limits<int>::max();
    while (!stack.empty())
    {
        // std::cout << "enter loop" << std::endl;
        IntPair currVertex = stack.top();
        stack.pop();
        IntPair prevVertex = predecessors[currVertex];
        // std::cout << "prev vertex " << prevVertex << " cur vertex " << currVertex << std::endl;

        int tempCost;
        if (prevVertex.first == -1)
        {
            tempCost = dynAlign(children[currVertex.first], trace.subspan(0, currVertex.second));
        }
        else
        {
            tempCost = dynAlign(children[currVertex.first], trace.subspan(prevVertex.second, currVertex.second - prevVertex.second));
        }

        int newCost = tempCost + vertexCosts[prevVertex];
        // std::cout << "new cost " << newCost << " prev vertex cost " << vertexCosts[prevVertex] << " temp cost " << tempCost << std::endl;
        if (newCost >= bestCost || newCost >= vertexCosts[currVertex])
        {
            // std::cout << "continue because newCost to high" << std::endl << std::endl;
            continue;
        }
        vertexCosts[currVertex] = newCost;

        // are we calculating the costs
        if (currVertex == finalVertex)
        {
            // std::cout << "reassignment of bestcost to: " << newCost << std::endl << std::endl;
            bestCost = newCost;
            continue;
        }

        // std::cout << "calculating next edges of " << currVertex << std::endl;
        for (const auto &nextEdge : outgoingEdges(currVertex, node, splitPositions))
        {
            // std::cout << nextEdge << " ";
            predecessors[nextEdge] = currVertex;
            stack.push(nextEdge);
        }
        // std::cout << std::endl << std::endl;

        // std::cout << "end iterations" << std::endl;
    }

    return bestCost;
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

// Implements _dyn_align_loop from Python using C++ constructs
// for traces of form R(QR)*
int dynAlignLoop(std::shared_ptr<TreeNode> node, std::span<const int> trace)
{
    auto &children = node->getChildren();

    if (children.size() != 2)
    {
        throw std::runtime_error("Loop node with id: " + node->getId() + " does not have exactly two children.");
    }

    int n = trace.size();
    if (n == 0)
    {
        return dynAlign(children[0], trace);
    }

    std::vector<IntPair> edges;
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

    std::unordered_map<IntPair, int, PairHash> qrCosts;
    for (const auto &edge : edges)
    {
        if (edge.first == edge.second)
        {
            qrCosts[edge] = 0;
            continue;
        }
        auto subTrace = trace.subspan(edge.first, edge.second - edge.first);
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
        rCosts[i] = dynAlign(children[0], trace.subspan(0, i));
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
