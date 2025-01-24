#include "TreeNode.h"
#include <iostream>
#include <memory>
#include <unordered_map>

void printTree(std::shared_ptr<TreeNode> node, int level = 0) {
    std::cout << std::string(level * 2, ' ') << "Node ID: " << node->getId() << ", Operation: " << node->getOperation() << std::endl;
    for (auto& child : node->getChildren()) {
        printTree(child, level + 1);
    }
}

std::shared_ptr<TreeNode> createExample() {
    auto root = std::make_shared<TreeNode>(SEQUENCE);

    auto child1 = std::make_shared<TreeNode>(PARALLEL);
    auto child2 = std::make_shared<TreeNode>(XOR);

    auto child3 = std::make_shared<TreeNode>(ACTIVITY, "a");
    auto child4 = std::make_shared<TreeNode>(ACTIVITY, "b");
    auto child5 = std::make_shared<TreeNode>(ACTIVITY, "c");

    child1->addChild(child3);
    child1->addChild(child4);

    child2->addChild(child5);

    root->addChild(child1);
    root->addChild(child2);

    return root;
}

int dynAlign(TreeNode& node, std::string& trace) 
{
    if (costTable.count(node.getId()) > 0) {
        if (costTable[node.getId()].count(trace) == 1) {
            std::cout << "Found trace: " << trace << " in node: " << node.getId() << std::endl;
            return costTable[node.getId()][trace];
        }
    }
    std::cout << "Trace: " << trace << " not found in node: " << node.getId() << std::endl;
    return -1;
}

void fillLetters(std::shared_ptr<TreeNode> node) {

    if (node->getOperation() == ACTIVITY || node->getOperation() == SILENT_ACTIVITY) {
        return;
    }   

    for (auto& child : node->getChildren()) {
        fillLetters(child);
        for (const auto& [key, value] : child->getLetters()) {
            node->getLetters()[key] = value;
        }
    }
}

int main()
{
    
    std::shared_ptr<TreeNode> root = createExample();
    printTree(root);
    std::string trace = "abc";
    fillLetters(root);

    costTable[1]["abc"] = 1;
    // dynAlign(root, trace);
    return 0;
}
