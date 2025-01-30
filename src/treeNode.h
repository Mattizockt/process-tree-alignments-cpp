#ifndef TREENODE_H
#define TREENODE_H

#include <unordered_map>
#include <string>
#include <vector>
#include <memory>

// node id -> (trace -> alignmentcost)
// later change it to an array for more efficiency
extern std::unordered_map<std::string, std::unordered_map<std::shared_ptr<std::vector<std::string>>, int>> costTable;

enum Operation
{
    SEQUENCE,
    PARALLEL,
    XOR,
    REDO_LOOP,
    ACTIVITY,
    SILENT_ACTIVITY
};

class TreeNode
{
public:
    TreeNode();
    TreeNode(Operation operation);
    TreeNode(Operation operation, std::string id);
    TreeNode(Operation operation, std::string activity, std::string id);

    static int
    getNumberOfNodes();

    std::string getId() const;
    void setId(std::string newId);

    Operation getOperation() const;
    void setOperation(Operation newOperation);

    std::string getActivity() const;

    std::unordered_map<std::string, bool> &getActivities();

    void addChild(std::shared_ptr<TreeNode> child);

    std::vector<std::shared_ptr<TreeNode>> &getChildren();

    void fillActivityMaps();

    void printTree(int level = 0);

private:
    static int numberOfNodes;
    std::string id;
    Operation operation;
    std::string activity;
    std::unordered_map<std::string, bool> activities;
    std::vector<std::shared_ptr<TreeNode>> children;
};

#endif // TREENODE_H