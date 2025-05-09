#ifndef TREENODE_H
#define TREENODE_H

#include <memory>
#include <string>
#include <unordered_set>
#include <unordered_map>
#include <vector>
#include <span>

struct SpanHash
{
    using is_transparent = void; // Enable transparent lookup

    // Hash for vector<int> keys
    std::size_t operator()(const std::vector<int> &vec) const
    {
        std::size_t seed = vec.size();
        for (auto &i : vec)
        {
            seed ^= i + 0x9e3779b9 + (seed << 6) + (seed >> 2);
        }
        return seed;
    }

    // Hash for span<const int> lookups
    std::size_t operator()(std::span<const int> span) const
    {
        std::size_t seed = span.size();
        for (auto &i : span)
        {
            seed ^= i + 0x9e3779b9 + (seed << 6) + (seed >> 2);
        }
        return seed;
    }
};

struct SpanEqual
{
    using is_transparent = void; // Enable transparent lookup

    // Compare vector<int> keys
    bool operator()(const std::vector<int> &lhs, const std::vector<int> &rhs) const
    {
        return lhs == rhs;
    }

    // Compare vector<int> with span<const int>
    bool operator()(const std::vector<int> &lhs, std::span<const int> rhs) const
    {
        if (lhs.size() != rhs.size())
            return false;
        return std::equal(lhs.begin(), lhs.end(), rhs.begin());
    }

    // Compare span<const int> with vector<int>
    bool operator()(std::span<const int> lhs, const std::vector<int> &rhs) const
    {
        if (lhs.size() != rhs.size())
            return false;
        return std::equal(lhs.begin(), lhs.end(), rhs.begin());
    }
};

// node id -> (trace -> alignmentcost)
extern std::unordered_map<int, std::unordered_map<std::vector<int>, int, SpanHash, SpanEqual>> costTable;

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
    TreeNode(Operation operation);

    TreeNode(Operation operation, int id);

    int getId() const;

    Operation getOperation() const;

    const std::unordered_set<int> &getActivities() const;

    void setActivities(std::unordered_set<int> newActivities);

    void addChild(std::shared_ptr<TreeNode> child);

    const std::vector<std::shared_ptr<TreeNode>> &getChildren() const;

    void fillActivityMaps();

    void printTree(int level = 0);

private:
    static int numberOfNodes;
    int id;
    Operation operation;
    std::unordered_set<int> activities;
    std::vector<std::shared_ptr<TreeNode>> children;
};

#endif // TREENODE_H