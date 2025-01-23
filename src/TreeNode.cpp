#include "TreeNode.h"

int TreeNode::numberOfNodes = 0;

TreeNode::TreeNode()
    : letters(), children(), activity(), operation(), id(++numberOfNodes)
{
}

TreeNode::TreeNode(Operation operation)
    : letters(), children(), activity(), operation(operation), id(++numberOfNodes)
{
}

int TreeNode::getNumberOfNodes() {
    return numberOfNodes;
}

int TreeNode::getId() const {
    return id;
}

void TreeNode::setId(int& newId) {
    id = newId;
}

Operation TreeNode::getOperation() const {
    return operation;
}

void TreeNode::setOperation(Operation& newOperation) {
    operation = newOperation;
}

std::string TreeNode::getActivity() const {
    return activity;
}

std::unordered_map<std::string, bool>& TreeNode::getLetters() {
    return letters;
}

std::vector<std::reference_wrapper<TreeNode>>& TreeNode::getChildren()  {
    return children;
}