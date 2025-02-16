#ifndef TREENODE_H
#define TREENODE_H

#include <unordered_map>
#include <string>
#include <vector>
#include <memory>

struct VectorHash {
    std::size_t operator()(const std::vector<std::string>& vec) const {
        std::size_t seed = vec.size();
        for (const auto& str : vec) {
            seed ^= std::hash<std::string>{}(str) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
        }
        return seed;
    }
};

// node id -> (trace -> alignmentcost)
// later change it to an array for more efficiency
extern std::unordered_map<std::string, std::unordered_map<std::vector<std::string>, int, VectorHash>> costTable;

enum Operation
{
    SEQUENCE, // 0
    PARALLEL, // 1
    XOR, // 2
    REDO_LOOP, // 3
    XOR_LOOP, // 4
    ACTIVITY, // 5
    SILENT_ACTIVITY // 6
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