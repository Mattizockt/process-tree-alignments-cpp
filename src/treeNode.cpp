#include "treeNode.h"
#include <iostream>
#include <unordered_map>
#include <memory>
#include "utils.h"

int TreeNode::numberOfNodes = 0;
std::unordered_map<std::string, std::unordered_map<std::string, int>> costTable;

TreeNode::TreeNode()
    : letters(), children(), activity(), operation(), id(std::to_string(++numberOfNodes))
{
}

TreeNode::TreeNode(Operation operation)
    : letters(), children(), activity(), operation(operation), id(std::to_string(++numberOfNodes))
{
}

TreeNode::TreeNode(Operation operation, std::string activity)
    : letters(), children(), activity(activity), operation(operation), id(std::to_string(++numberOfNodes))
{
    if (operation == ACTIVITY)
    {
        letters[activity] = true;
    }
}

TreeNode::TreeNode(Operation operation, std::string activity, std::string id)
    : letters(), children(), activity(activity), operation(operation), id(id)
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
