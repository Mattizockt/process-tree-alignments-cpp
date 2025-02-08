#include <memory.h>
#include <string>
#include <iostream>
#include "treeNode.h"
#include "utils.h"
#include "parser.h"

int main(int argc, char* argv[])
{
    std::cout << "Number of arguments: " << argc << std::endl;
    for (int i = 0; i < argc; ++i) {
        std::cout << "Argument " << i << ": " << argv[i] << std::endl;
    }

    const std::string xesPath = "../data/xes/"; // Relative xesPath to directory
    const std::string ptmlPath = "../data/ptml/";
    const std::string outputFileName = "../output/alignCost.json";

    // relative path from src directory
    parseAndAlign(xesPath, ptmlPath, outputFileName);
}
