#include "TreeNode.h"
#include <iostream>
#include <unordered_map>
#include <memory> 

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
    if (operation == ACTIVITY) {
        letters[activity] = true;
    }
}

int TreeNode::getNumberOfNodes() {
    return numberOfNodes;
}

int TreeNode::getId() const {
    return id;
}

void TreeNode::setId(int newId) {
    id = newId;
}

Operation TreeNode::getOperation() const {
    return operation;
}

void TreeNode::setOperation(Operation newOperation) {
    operation = newOperation;
}

std::string TreeNode::getActivity() const {
    return activity;
}

void TreeNode::addChild(std::shared_ptr<TreeNode> child) {
    children.push_back(child);
}

std::unordered_map<std::string, bool>& TreeNode::getLetters() {
    return letters;
}

std::vector<std::shared_ptr<TreeNode>>& TreeNode::getChildren()  {
    return children;
}