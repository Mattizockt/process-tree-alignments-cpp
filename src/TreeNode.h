#ifndef TREENODE_H
#define TREENODE_H

#include <unordered_map>
#include <string>
#include <vector>
#include <memory>

extern std::unordered_map<int, std::unordered_map<std::string, int>> costTable;

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
    TreeNode(Operation operation, std::string activity);

    static int getNumberOfNodes();

    int getId() const;
    void setId(int newId);

    Operation getOperation() const;
    void setOperation(Operation newOperation);

    std::string getActivity() const;

    std::unordered_map<std::string, bool> &getLetters();

    void addChild(std::shared_ptr<TreeNode> child);

    std::vector<std::shared_ptr<TreeNode>> &getChildren();

    void fillLetterMaps();

    void printTree(int level = 0);

private:
    static int numberOfNodes;
    int id;
    Operation operation;
    std::string activity;
    std::unordered_map<std::string, bool> letters;
    std::vector<std::shared_ptr<TreeNode>> children;
};

#endif // TREENODE_H