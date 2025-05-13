#ifndef BINDINGS_H
#define BINDINGS_H
#include "parser.h"
#include <memory>
#include <string>
#include <pybind11/stl.h>


class AlignmentWrapper
{
private:
    std::shared_ptr<TreeNode> processTree;

public:
    AlignmentWrapper();

    size_t align(const std::vector<std::string> newTrace) const;

    void loadTree(std::string treePath);

};

#endif