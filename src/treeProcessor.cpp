#include "treeProcessor.h"
#include <iostream>
#include <unordered_map>
#include <memory>
#include "utils.h"
#include <iostream>
#include <unordered_set>
#include <numeric>
#include <limits>

int TreeNode::numberOfNodes = 0;
std::unordered_map<int, std::unordered_map<std::string, int>> costTable;

TreeNode::TreeNode()
    : letters(), children(), activity(), operation(), id(++numberOfNodes)
{
}

TreeNode::TreeNode(Operation operation)
    : letters(), children(), activity(), operation(operation), id(++numberOfNodes)
{
}

TreeNode::TreeNode(Operation operation, std::string activity)
    : letters(), children(), activity(activity), operation(operation), id(++numberOfNodes)
{
    if (operation == ACTIVITY)
    {
        letters[activity] = true;
    }
}

int TreeNode::getNumberOfNodes()
{
    return numberOfNodes;
}

int TreeNode::getId() const
{
    return id;
}

void TreeNode::setId(int newId)
{
    id = newId;
}

Operation TreeNode::getOperation() const
{
    return operation;
}

void TreeNode::setOperation(Operation newOperation)
{
    operation = newOperation;
}

std::string TreeNode::getActivity() const
{
    return activity;
}

void TreeNode::addChild(std::shared_ptr<TreeNode> child)
{
    children.push_back(child);
}

std::unordered_map<std::string, bool> &TreeNode::getLetters()
{
    return letters;
}

std::vector<std::shared_ptr<TreeNode>> &TreeNode::getChildren()
{
    return children;
}

void TreeNode::fillLetterMaps()
{

    if (this->getOperation() == ACTIVITY || this->getOperation() == SILENT_ACTIVITY)
    {
        return;
    }

    for (auto &child : this->getChildren())
    {
        child->fillLetterMaps();
        for (const auto &[key, value] : child->getLetters())
        {
            this->getLetters()[key] = value;
        }
    }
}

void TreeNode::printTree(int level)
{
    std::cout << std::string(level * 2, ' ') << "Node ID: " << this->getId() << ", Operation: " << this->getOperation() << ", Activity (if exists): " << this->activity << std::endl;
    for (auto &child : this->getChildren())
    {
        child->printTree(level + 1);
    }
}

int dynAlign(std::shared_ptr<TreeNode> node, const std::string &trace);

int dynAlignActivity(std::shared_ptr<TreeNode> node, const std::string &trace)
{
    if (trace.find(node->getActivity()) == std::string::npos)
    {
        return trace.length() + 1;
    }
    else
    {
        return trace.length() - 1;
    }
}

// TODO write tests
int dynAlignSilentActivity(std::shared_ptr<TreeNode> node, const std::string &trace)
{
    return trace.length();
}

// TODO write tests
int dynAlignSequence(std::shared_ptr<TreeNode> node, const std::string &trace)
{
    if (trace.length() == 0)
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
        std::vector<std::string> splittedTraces = segmentTrace(trace, split);
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

int dynAlignXor(std::shared_ptr<TreeNode> node, const std::string &trace)
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

int dynAlignParallel(std::shared_ptr<TreeNode> node, const std::string &trace)
{
    const auto &children = node->getChildren();
    std::vector<std::string> subTraces(children.size(), "");
    int unmatched = 0;

    for (char c : trace) {
        bool matched = false;
        for (size_t i = 0; i < children.size(); i++) {
            if (children[i]->getLetters().count(std::string(1, c)) == 1) {
                subTraces[i] += c;
                matched = true;
                break;
            }
        }
        if (!matched) {
            unmatched++;
        }
    }

    int cost = 0;
    for (size_t i = 0; i < children.size(); i++) {
        cost += dynAlign(children[i], subTraces[i]);
    }

    cost += unmatched;

    return cost;
}

int dynAlignLoop(std::shared_ptr<TreeNode> node, const std::string &trace)
{
    return 1;
}

int dynAlign(std::shared_ptr<TreeNode> node, const std::string &trace)
{
    if (costTable.count(node->getId()) > 0)
    {
        if (costTable[node->getId()].count(trace) == 1)
        {
            std::cout << "Found trace: " << trace << " in node: " << node->getId() << std::endl;
            return costTable[node->getId()][trace];
        }
    }

    // ad to cost table
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
        // TODO: THROW EXCEPTION LATER
        std::cout << "Unknown operation" << std::endl;
        return -1;
    }

    costTable[node->getId()][trace] = costs;
    // std::cout << "Trace: " << trace << " not found in node: " << node->getId() << std::endl;
    return costs;
}
