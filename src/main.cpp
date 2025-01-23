#include <iostream>
#include "TreeNode.h"

int main()
{
    // Create root node
    TreeNode root(SEQUENCE);

    // Create child nodes
    TreeNode child1(PARALLEL);
    TreeNode child2(ACTIVITY);
    TreeNode child3(ACTIVITY);
    TreeNode child4(ACTIVITY);
    TreeNode child5(ACTIVITY);

    // Add children to root
    root.getChildren().push_back(std::ref(child1));
    root.getChildren().push_back(std::ref(child2));

    // Add children to child1
    child1.getChildren().push_back(std::ref(child3));
    child1.getChildren().push_back(std::ref(child4));

    // Add child to child2
    child2.getChildren().push_back(std::ref(child5));

    // Print the tree structure
    std::cout << "Root ID: " << root.getId() << ", Operation: " << root.getOperation() << std::endl;
    for (auto &child : root.getChildren())
    {
        std::cout << "  Child ID: " << child.get().getId() << ", Operation: " << child.get().getOperation() << ", Number of children " << child.get().getChildren().size() << std::endl;
        for (auto &grandchild : child.get().getChildren())
        {
            std::cout << "    Grandchild ID: " << grandchild.get().getId() << ", Operation: " << grandchild.get().getOperation() << std::endl;
        }
    }

    return 0;
}
