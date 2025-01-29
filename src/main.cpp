#include <memory.h>
#include <string>
#include "treeNode.h"
#include "utils.h"
#include "parser.h"

int main()
{

    // createPtmlXesPairs();

    // const std::string xesPath = "../data/xes/exampleTraces.xes";
    // parseXes(xesPath);

    const std::string ptmlPath = "../data/ptml/running-example-just-two-cases_pt00.ptml";
    parsePtml(ptmlPath);
}
