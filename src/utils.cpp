#include "TreeNode.h"
#include <memory>
#include <unordered_set>

std::string pruneInputTrace(std::shared_ptr<TreeNode> node, std::string &trace)
{
    std::unordered_set<char> nodeLetters;
    for (const auto &pair : node->getLetters())
    {
        nodeLetters.insert(pair.first[0]);
    }

    std::string prunedTrace;
    for (char c : trace)
    {
        if (nodeLetters.count(c) > 0)
        {
            prunedTrace.push_back(c);
        }
    }

    return prunedTrace;
}

std::shared_ptr<TreeNode> createExample()
{
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
