#include "treeNode.h"
#include "utils.h"
#include <iostream>
#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <numeric>
#include <limits>

// TODO write tests
int dynAlignActivity(std::shared_ptr<TreeNode> node, const std::string &trace) {
    if (trace.find(node->getActivity()) == std::string::npos) {
        return trace.length() + 1;
    }
    else {
        return trace.length() - 1;
    }
}

// TODO write tests
int dynAlignSilentActivity(std::shared_ptr<TreeNode> node, const std::string &trace) {
    return trace.length();
}

// TODO write tests
int dynAlignSequence(std::shared_ptr<TreeNode> node, const std::string &trace) {
    if (trace.length() == 0) {
        const auto &children = node->getChildren();
        return std::accumulate(children.begin(), children.end(), 0, [&trace](int sum, const auto &child) {
            return sum + dynAlign(child, trace);
        });
    }

    int minCosts = std::numeric_limits<int>::max();

    
    return -1;
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
    
    int costs;
    switch (node->getOperation())
    {
    case SEQUENCE:
    case PARALLEL:
    case XOR:
    case REDO_LOOP:
    case ACTIVITY:
        costs = dynAlignActivity(node, trace);
        break;
    case SILENT_ACTIVITY:
        costs = dynAlignSilentActivity(node, trace);
        break;
    default:
        // TODO: THROW EXCEPTION LATER
        std::cout << "Unknown operation" << std::endl;
        break;
    }

    std::cout << "Trace: " << trace << " not found in node: " << node->getId() << std::endl;
    return -1;
}

int main()
{   
    std::shared_ptr<TreeNode> root = createExample();
    root->fillLetterMaps();
    root->printTree();

    // std::string trace = "asdhbcgda";
    // trace = pruneInputTrace(root, trace);
    // std::cout << "Pruned trace: " << trace << std::endl;

    std::string trace = "dbcda";

    possibleSplits(root, trace);
    dynAlign(root, trace);
    return 0;
}
