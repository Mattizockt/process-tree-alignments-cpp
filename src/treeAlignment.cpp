#include "treeNode.h"
#include "utils.h"
#include <memory>
#include <string>
#include <numeric>
#include <limits>
#include <vector>
#include <algorithm>
#include <iostream>

using StringVec = std::vector<std::string>;

int dynAlign(std::shared_ptr<TreeNode> node, const std::shared_ptr<StringVec> trace);

int dynAlignActivity(std::shared_ptr<TreeNode> node, std::shared_ptr<StringVec> trace)
{
    // std::cout << "activity" << std::endl;
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

int dynAlignSilentActivity(std::shared_ptr<TreeNode> node, const std::shared_ptr<StringVec> trace)
{
    // std::cout << "silent activity" << std::endl;
    return trace->size();
}

std::vector<std::pair<int, int>> get_segments_for_sequence(std::shared_ptr<StringVec> trace, std::shared_ptr<TreeNode> node)
{
    if (node->getChildren().size() != 2)
        throw std::runtime_error("get_segments_for_sequence not implemented for more/less than two children.");

    int traceSize = trace->size();
    std::vector<std::pair<int, int>> segments = {
        {0, traceSize},
        {traceSize, 0}};

    std::vector<int> splitPositions;
    for (int i = 1; i < traceSize; i++)
    {
        if (node->getChildren()[1]->getActivities().count(trace->at(i)) &&
            node->getChildren()[0]->getActivities().count(trace->at(i - 1)))
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

struct PairCost {
    std::pair<int, int> first_pair;
    std::pair<int, int> second_pair;
    int cost;

    PairCost(std::pair<int, int> fp, std::pair<int, int> sp, int c)
        : first_pair(fp), second_pair(sp), cost(c) {}

    PairCost() : first_pair({-1, -1}), second_pair({-1, -1}), cost(-1) {}
};

std::vector<PairCost> outgoingEdges(std::pair<int, int> v, int numChild, int n, std::shared_ptr<StringVec> trace, std::shared_ptr<TreeNode> node, int upperBound)
{
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
        if (k < n - 1 && node->getChildren().at(v.first)->getActivities().count(trace->at(k)))
        {
            continue;
        }
        std::shared_ptr<StringVec> subTrace = std::make_shared<StringVec>(trace->begin() + v.second, trace->begin() + k);
        int tempCost = dynAlign(node->getChildren().at(v.first), subTrace);
        if (tempCost > upperBound)
        {
            continue;
        }
        result.push_back(PairCost(std::pair<int, int>(v.first, v.second),std::pair<int, int>(v.first+1, k), tempCost));
    }
    return result;
}

int dynAlignSequence(std::shared_ptr<TreeNode> node, const std::shared_ptr<StringVec> trace)
{
    // std::cout << "sequence" << std::endl;
    const auto &children = node->getChildren();
    if (trace->size() == 0)
    {
        return std::accumulate(children.begin(), children.end(), 0, [&trace](int sum, const auto &child)
                               { return sum + dynAlign(child, trace); });
    }

    int pos = 0;
    int old_pos = 0;
    int costs = 0;

    if (trace->size() > children.size() &&
        children[0]->getActivities().count(trace->at(0)) &&
        children.back()->getActivities().count(trace->back()))
    {
        for (const auto &child : children)
        {
            while (pos < trace->size() && child->getActivities().count(trace->at(pos)))
            {
                pos += 1;
            }
            std::shared_ptr<StringVec> subTrace = std::make_shared<StringVec>(trace->begin() + old_pos, trace->begin() + pos);
            costs += dynAlign(child, subTrace);
            old_pos = pos;
        }
    }

    if (pos < trace->size())
    {
        costs = std::numeric_limits<int>::max();
    }

    if (costs == 0)
    {
        return 0;
    }

    if (children.size() == 2)
    {
        auto segments = get_segments_for_sequence(trace, node);
        // std::cout << "other size: " << std::to_string(segments.size()) << std::endl;
        for (const auto segment : segments)
        {
            int split = segment.first;
            std::shared_ptr<StringVec> first_part = std::make_shared<StringVec>(trace->begin(), trace->begin() + split);
            std::shared_ptr<StringVec> second_part = std::make_shared<StringVec>(trace->begin() + split, trace->end());

            auto leftCost = dynAlign(children[0], first_part);
            if (leftCost >= costs)
            {
                continue;
            }
            auto rightCost = dynAlign(children[1], second_part);

            costs = std::min(leftCost + rightCost, costs);
        }
        return costs;
    }

    std::vector<std::pair<int, int>> vertices;

    int numChildren = node->getChildren().size();
    int n = trace->size();
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

    std::unordered_map<std::pair<int, int>, int, PairHash> dijkstraCosts;

    for (const auto vertex : vertices)
    {
        dijkstraCosts[vertex] = std::numeric_limits<int>::max();
    }
    dijkstraCosts[start] = 0;
    std::unordered_map<std::pair<int, int>, bool, PairHash> visited;

    while (visited.size() < vertices.size())
    {
        std::pair<int, int> current;
        int min_cost = std::numeric_limits<int>::max();

        for (const auto &v : vertices)
        {
            // this implementation could cause problems later on, rewrite later TODO
            if (visited.find(v) == visited.end() && dijkstraCosts[v] <= min_cost)
            {
                min_cost = dijkstraCosts[v];
                current = v;
            }
        }
        visited[current] = true;
        if (dijkstraCosts[current] > costs)
        {
            continue;
        }
        // TODO find other way for constantly using std:;numeric_limiits
        for (auto edge : outgoingEdges(current, numChildren, n, trace, node, costs)) {
            if (dijkstraCosts[current] != std::numeric_limits<int>::max()) {
                dijkstraCosts[edge.second_pair] = std::min(dijkstraCosts[edge.second_pair], dijkstraCosts[current] + edge.cost);
            }
            else {
                dijkstraCosts[edge.second_pair] = dijkstraCosts[edge.second_pair];
            }
        }
    }

    return std::min(costs, dijkstraCosts[end]);
}

int dynAlignXor(std::shared_ptr<TreeNode> node, const std::shared_ptr<StringVec> trace)
{
    // std::cout << "xor" << std::endl;
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

int dynAlignParallel(std::shared_ptr<TreeNode> node, const std::shared_ptr<StringVec> trace)
{
    // std::cout << "parallel" << std::endl;
    const auto &children = node->getChildren();
    std::vector<std::shared_ptr<StringVec>> subTraces(children.size());
    std::generate(subTraces.begin(), subTraces.end(), []
                  { return std::make_shared<StringVec>(); });

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

int dynAlignLoop(std::shared_ptr<TreeNode> node, const std::shared_ptr<StringVec> trace)
{
    // std::cout << "loop" << std::endl;

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
        auto subTrace = std::make_shared<StringVec>(trace->begin() + edge.first, trace->begin() + edge.second);
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
        rCosts[i] = dynAlign(children[0], std::make_shared<StringVec>(trace->begin(), trace->begin() + i));
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
int dynAlignXorLoop(std::shared_ptr<TreeNode> node, const std::shared_ptr<StringVec> trace)
{
    // std::cout << "xor loop" << std::endl;
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

int dynAlign(std::shared_ptr<TreeNode> node, const std::shared_ptr<StringVec> trace)
{
    if (costTable.count(node->getId()) > 0)
    {
        if (costTable[node->getId()].count(*trace) == 1)
        {
            // std::cout << "costtable" << std::endl;
            return costTable[node->getId()][*trace];
        }
        else
        {
            // std::cout << "not found" << std::endl;
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

    costTable[node->getId()][*trace] = costs;
    return costs;
}
