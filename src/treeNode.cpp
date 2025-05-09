#include "parser.h"
#include "treeNode.h"
#include "utils.h"
#include <iostream>
#include <memory>
#include <unordered_map>
#include <unordered_set>

int TreeNode::numberOfNodes = 0;

std::unordered_map<int, std::unordered_map<std::vector<int>, int, SpanHash, SpanEqual>> costTable;

TreeNode::TreeNode(Operation operation)
    : activities(), children(), operation(operation), id(++numberOfNodes)
{
    if (operation == ACTIVITY) {
        activities.insert(this->getId());
    }
}

TreeNode::TreeNode(Operation operation, int id)
    : activities(), children(), operation(operation), id(id)
{
        if (operation == ACTIVITY) {
        activities.insert(id);
    }
}

int TreeNode::getId() const
{
    return id;
}

Operation TreeNode::getOperation() const
{
    return operation;
}

void TreeNode::setActivities(std::unordered_set<int> newActivities)
{
    activities = newActivities;
}

void TreeNode::addChild(std::shared_ptr<TreeNode> child)
{
    children.push_back(child);
}

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

void TreeNode::printTree(int level)
{
    std::string activityName = "None"; // Default if activity is -1 or not found

    if (this->operation == ACTIVITY) {
        auto it = idToActivity.find(this->getId());
        if (it != idToActivity.end()) {
            activityName = it->second; // Found the activity, get its name
        } else {
            activityName = "Unknown Activity (ID: " + std::to_string(this->getId()) + ")";
        }
    }

    std::cout << std::string(level * 2, ' ') << "Node ID: " << this->getId() << ", Operation: " << this->getOperation() << ", Activity (if exists): " << activityName << std::endl;

    for (auto &child : this->getChildren())
    {
        child->printTree(level + 1);
    }
}