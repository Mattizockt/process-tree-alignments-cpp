#include "TreeNode.h"
#include "utils.h"
#include <iostream>
#include <memory>
#include <unordered_map>
#include <unordered_set>

int dynAlign(std::shared_ptr<TreeNode> node, std::string &trace)
{
    if (costTable.count(node->getId()) > 0)
    {
        if (costTable[node->getId()].count(trace) == 1)
        {
            std::cout << "Found trace: " << trace << " in node: " << node->getId() << std::endl;
            return costTable[node->getId()][trace];
        }
    }

    std::cout << "Trace: " << trace << " not found in node: " << node->getId() << std::endl;
    return -1;
}

int main()
{

    std::shared_ptr<TreeNode> root = createExample();
    root->fillLetterMaps();
    root->printTree();

    std::string trace = "asdhbcg";
    trace = pruneInputTrace(root, trace);
    std::cout << "Pruned trace: " << trace << std::endl;

    dynAlign(root, trace);
    return 0;
}
