#include <memory.h>
#include <string>
#include "treeNode.h"
#include "utils.h"
#include "parser.h"

int main()
{
    const std::string xesPath = "../data/xes/"; // Relative xesPath to directory
    const std::string ptmlPath = "../data/ptml/";

    // relative path from src directory
    createPtmlXesPairs(xesPath, ptmlPath);
}
