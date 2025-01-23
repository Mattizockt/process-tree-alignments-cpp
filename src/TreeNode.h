#ifndef TREENODE_H
#define TREENODE_H

#include <unordered_map>
#include <string>
#include <vector>

extern std::unordered_map<std::string, std::unordered_map<std::string, int>> costTable;

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

    static int getNumberOfNodes();

    int getId() const;
    void setId(int& newId);

    Operation getOperation() const;
    void setOperation(Operation& newOperation);

    std::string getActivity() const;

    std::unordered_map<std::string, bool>& getLetters();

    std::vector<std::reference_wrapper<TreeNode>>& getChildren();

private:
    static int numberOfNodes;
    int id;
    Operation operation;
    std::string activity;
    std::unordered_map<std::string, bool> letters;
    std::vector<std::reference_wrapper<TreeNode>> children;
};

#endif // TREENODE_H