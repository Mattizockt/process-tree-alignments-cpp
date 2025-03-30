#ifndef TREENODE_H
#define TREENODE_H

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

struct VectorHash {
    std::size_t operator()(std::vector<int> const& vec) const {
        std::size_t seed = vec.size();
        for(auto& i : vec) {
          seed ^= i + 0x9e3779b9 + (seed << 6) + (seed >> 2);
        }
        return seed;
      }
};


// node id -> (trace -> alignmentcost)
// TODOlater change it to an array for more efficiency
extern std::unordered_map<std::string, std::unordered_map<std::vector<int>, int, VectorHash>> costTable;

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
    TreeNode(Operation operation, int activity);
    TreeNode(Operation operation, int activity, std::string id);

    static int
    getNumberOfNodes();

    std::string getId() const;
    void setId(std::string newId);

    Operation getOperation() const;
    void setOperation(Operation newOperation);

    int getActivity() const;

    std::unordered_map<int, bool> &getActivities();

    void addChild(std::shared_ptr<TreeNode> child);

    std::vector<std::shared_ptr<TreeNode>> &getChildren();

    void fillActivityMaps();

    void printTree(int level = 0);

private:
    static int numberOfNodes;
    std::string id;
    Operation operation;
    int activity;
    std::unordered_map<int, bool> activities;
    std::vector<std::shared_ptr<TreeNode>> children;
};

#endif // TREENODE_H