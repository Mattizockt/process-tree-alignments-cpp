#include <memory.h>
#include <string>
#include <iostream>
#include "parser.h"

int main(int argc, char *argv[])
{
    const std::string xesPath = "../data/xes/"; // Relative paths from working dir
    const std::string ptmlPath = "../data/ptml/";

    parseAndAlign(xesPath, ptmlPath);
}