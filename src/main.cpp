#include <memory.h>
#include <string>
#include <iostream>
#include "treeNode.h"
#include "utils.h"
#include "parser.h"

int main(int argc, char* argv[])
{
    const std::string xesPath = "../data/xes/"; // Relative xesPath to directory from working dir
    const std::string ptmlPath = "../data/ptml/";
    
    std::string outputFile = "../output/defaultOutput.json";
    // relative path from src directory
    if (argc == 2) {
        outputFile = argv[1];
    }
    parseAndAlign(xesPath, ptmlPath, outputFile);
}