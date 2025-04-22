#ifndef BINDINGS_H
#define BINDINGS_H
#include "parser.h"
#include <memory>
#include <string>

class AlignmentWrapper
{
private:
    std::vector<int> traceContent;
    std::shared_ptr<TreeNode> processTree;

public:
    AlignmentWrapper();

    void setTrace(const std::vector<std::string> newTrace);

    void loadTree(std::string treePath);

    size_t align() const;
};

#endif