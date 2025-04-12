#include "parser.h"
#include "treeNode.h"
#include "utils.h"
#include <iostream>
#include <memory>
#include <unordered_map>

int TreeNode::numberOfNodes = 0;

std::unordered_map<std::string, std::unordered_map<std::vector<int>, int, SpanHash, SpanEqual>> costTable;

TreeNode::TreeNode()
    : activities(), children(), activity(), operation(), id(std::to_string(++numberOfNodes))
{
}

TreeNode::TreeNode(Operation operation)
    : activities(), children(), activity(), operation(operation), id(std::to_string(++numberOfNodes))
{
}

TreeNode::TreeNode(Operation operation, int activity)
    : activities(), children(), activity(activity), operation(operation), id(std::to_string(++numberOfNodes))
{
    if (operation == ACTIVITY)
    {
        activities[activity] = true;
    }
}

TreeNode::TreeNode(Operation operation, int activity, std::string id)
    : activities(), children(), activity(activity), operation(operation), id(id)
{
    if (operation == ACTIVITY)
    {
        activities[activity] = true;
    }
}

int TreeNode::getNumberOfNodes()
{
    return numberOfNodes;
}

std::string TreeNode::getId() const
{
    return id;
}

void TreeNode::setId(std::string newId)
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

int TreeNode::getActivity() const
{
    return activity;
}

void TreeNode::addChild(std::shared_ptr<TreeNode> child)
{
    children.push_back(child);
}

// all activities of this node and its successor
std::unordered_map<int, bool> &TreeNode::getActivities()
{
    return activities;
}

std::vector<std::shared_ptr<TreeNode>> &TreeNode::getChildren()
{
    return children;
}

void TreeNode::fillActivityMaps()
{

    if (this->getOperation() == ACTIVITY || this->getOperation() == SILENT_ACTIVITY)
    {
        return;
    }

    for (auto &child : this->getChildren())
    {
        child->fillActivityMaps();
        for (const auto &[key, value] : child->getActivities())
        {
            this->getActivities()[key] = value;
        }
    }
}

void TreeNode::printTree(int level)
{
    std::string activityName = this->activity != -1 ? activityVector[this->activity] : "None";

    std::cout << std::string(level * 2, ' ') << "Node ID: " << this->getId() << ", Operation: " << this->getOperation() << ", Activity (if exists): " << activityName << std::endl;
    for (auto &child : this->getChildren())
    {
        child->printTree(level + 1);
    }
}
