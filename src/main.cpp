#include <memory.h>
#include <string>
#include <iostream>
#include "treeNode.h"
#include "utils.h"
#include "parser.h"

int main(int argc, char *argv[])
{
    const std::string xesPath = "../data/xes/"; // Relative paths from working dir
    const std::string ptmlPath = "../data/ptml/";

    parseAndAlign(xesPath, ptmlPath);
}