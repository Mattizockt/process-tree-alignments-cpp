#ifndef TREENODE_H
#define TREENODE_H

#include <memory>
#include <string>
#include <unordered_set>
#include <unordered_map>
#include <vector>
#include <span>
#include <functional>
#include <iostream>

    // Custom hash function for vector<int> and span<const int>
    struct SpanHash
    {
        using is_transparent = void; // Enable transparent lookup
        std::size_t operator()(const std::vector<int> &vec) const;
        std::size_t operator()(std::span<const int> vec) const;
    };

    // Custom equality comparison for vector<int> and span<const int>
    struct SpanEqual
    {
        using is_transparent = void; // Enable transparent lookup
        bool operator()(const std::vector<int> &lhs, const std::vector<int> &rhs) const;
        bool operator()(const std::vector<int> &lhs, std::span<const int> rhs) const;
        bool operator()(std::span<const int> lhs, const std::vector<int> &rhs) const;
    };

// node id -> (trace -> alignmentcost)
extern std::unordered_map<std::string, std::unordered_map<std::vector<int>, int, SpanHash, SpanEqual>> costTable;

enum Operation
{
    SEQUENCE,       // 0
    PARALLEL,       // 1
    XOR,            // 2
    REDO_LOOP,      // 3
    XOR_LOOP,       // 4
    ACTIVITY,       // 5
    SILENT_ACTIVITY // 6
};

class TreeNode
{
public:
    // TreeNode();
    TreeNode(Operation operation);
    TreeNode(Operation operation, int activity);
    TreeNode(Operation operation, std::string id);
    TreeNode(Operation operation, int activity, std::string id);

    // static int
    // getNumberOfNodes();

    std::string getId() const;
    // void setId(std::string newId);

    Operation getOperation() const;
    // void setOperation(Operation newOperation);

    int getActivity() const;

    const std::unordered_set<int> &getActivities() const;

    void setActivities(std::unordered_set<int> newActivities);

    void addChild(std::shared_ptr<TreeNode> child);

    const std::vector<std::shared_ptr<TreeNode>> &getChildren() const;

    void fillActivityMaps();

    // void printTree(int level = 0);

private:
    static int numberOfNodes;
    std::string id;
    Operation operation;
    int activity;
    std::unordered_set<int> activities;
    std::vector<std::shared_ptr<TreeNode>> children;
};

#endif // TREENODE_H