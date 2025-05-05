#include "parser.h"
#include "treeNode.h"
#include "utils.h"
#include <iostream>
#include <memory>
#include <unordered_map>
#include <unordered_set>

// Hash for vector<int> keys
std::size_t SpanHash::operator()(const std::vector<int> &vec) const
{
    // Main hash
    size_t hash = 5381;

    // Add sequential hash
    for (size_t i = 0; i < vec.size(); ++i)
    {
        hash = ((hash << 5) + hash) + vec[i];
    }

    // XOR with pattern signature
    if (!vec.empty())
    {
        hash ^= (vec[0] * 7919) << 16;
        hash ^= (vec[vec.size() - 1] * 8731) << 8;
    }

    // Mix in sequence property
    hash ^= vec.size() * 31;

    return hash;
}

// Hash for span<const int> lookups
std::size_t SpanHash::operator()(std::span<const int> vec) const
{
    // Main hash
    size_t hash = 5381;

    // Add sequential hash
    for (size_t i = 0; i < vec.size(); ++i)
    {
        hash = ((hash << 5) + hash) + vec[i];
    }

    // XOR with pattern signature
    if (!vec.empty())
    {
        hash ^= (vec[0] * 7919) << 16;
        hash ^= (vec[vec.size() - 1] * 8731) << 8;
    }

    // Mix in sequence property
    hash ^= vec.size() * 31;

    return hash;
}

// using is_transparent = void; // Enable transparent lookup

// Compare vector<int> keys
bool SpanEqual::operator()(const std::vector<int> &lhs, const std::vector<int> &rhs) const
{
    return lhs == rhs;
}

// Compare vector<int> with span<const int>
bool SpanEqual::operator()(const std::vector<int> &lhs, std::span<const int> rhs) const
{
    if (lhs.size() != rhs.size())
        return false;
    return std::equal(lhs.begin(), lhs.end(), rhs.begin());
}

// Compare span<const int> with vector<int>
bool SpanEqual::operator()(std::span<const int> lhs, const std::vector<int> &rhs) const
{
    if (lhs.size() != rhs.size())
        return false;
    return std::equal(lhs.begin(), lhs.end(), rhs.begin());
}

int TreeNode::numberOfNodes = 0;

std::unordered_map<std::string, std::unordered_map<std::vector<int>, int, SpanHash, SpanEqual>> costTable;

// TreeNode::TreeNode()
//     : activities(), children(), activity(), operation(), id(std::to_string(++numberOfNodes))
// {
// }

TreeNode::TreeNode(Operation operation)
    : activities(), children(), activity(), operation(operation), id(std::to_string(++numberOfNodes))
{
}

TreeNode::TreeNode(Operation operation, std::string id)
    : activities(), children(), activity(), operation(operation), id(id)
{
}

TreeNode::TreeNode(Operation operation, int activity)
    : activities(), children(), activity(activity), operation(operation), id(std::to_string(++numberOfNodes))
{
    if (operation == ACTIVITY)
    {
        activities.insert(activity);
    }
}

TreeNode::TreeNode(Operation operation, int activity, std::string id)
    : activities(), children(), activity(activity), operation(operation), id(id)
{
    if (operation == ACTIVITY)
    {
        activities.insert(activity);
    }
}

// int TreeNode::getNumberOfNodes()
// {
//     return numberOfNodes;
// }

std::string TreeNode::getId() const
{
    return id;
}

// void TreeNode::setId(std::string newId)
// {
//     id = newId;
// }

Operation TreeNode::getOperation() const
{
    return operation;
}

// void TreeNode::setOperation(Operation newOperation)
// {
//     operation = newOperation;
// }

int TreeNode::getActivity() const
{
    return activity;
}

void TreeNode::setActivities(std::unordered_set<int> newActivities)
{
    activities = newActivities;
}

void TreeNode::addChild(std::shared_ptr<TreeNode> child)
{
    children.push_back(child);
}

// all activities of this node and its successor
const std::unordered_set<int> &TreeNode::getActivities() const
{
    return activities;
}

const std::vector<std::shared_ptr<TreeNode>> &TreeNode::getChildren() const
{
    return children;
}

void TreeNode::fillActivityMaps()
{

    if (this->getOperation() == ACTIVITY || this->getOperation() == SILENT_ACTIVITY)
    {
        return;
    }

    auto &currActivities = this->activities;
    for (auto &child : this->getChildren())
    {
        child->fillActivityMaps();
        for (const auto &activity : child->getActivities())
        {
            currActivities.insert(activity);
        }
    }
}

// void TreeNode::printTree(int level)
// {
//     std::string activityName = this->activity != -1 ? activityVector[this->activity] : "None";

//     std::cout << std::string(level * 2, ' ') << "Node ID: " << this->getId() << ", Operation: " << this->getOperation() << ", Activity (if exists): " << activityName << std::endl;
//     for (auto &child : this->getChildren())
//     {
//         child->printTree(level + 1);
//     }
// }
