#include "bindings.h"
#include "parser.h"
#include "treeAlignment.h"
#include "utils.h"
#include <iostream>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <span>
#include <string>

namespace py = pybind11;

AlignmentWrapper::AlignmentWrapper()
{
}

void AlignmentWrapper::setTrace(const std::vector<std::string> newTrace)
{
    std::vector<int> intTrace = convertStringTrace(newTrace);
    traceContent = intTrace;
}

void AlignmentWrapper::loadTree(std::string treePath)
{
    processTree = parsePtml(treePath);
}

size_t AlignmentWrapper::align() const
{
    std::span<const int> trace = traceContent;
    costTable.clear();
    // traceCounter.clear();
    a = trace;

    // std::cout << visualizeSpanTrace(trace) << std::endl;

    // int n = trace.size(); // Example value
    // std::vector<int> vec;
    // for (int i = 0; i < n; i++) {
    //     vec.push_back(i);
    // }

    const size_t cost = dynAlign(processTree, trace);


    // for (int i = 0; i < n; ++i) {
    //     auto it = traceCounter.find(i);
    //     if (it != traceCounter.end()) {
    //         std::cout << it->second;
    //     } else {
    //         std::cout << 0;
    //     }

    //     if (i < 100) {
    //         std::cout << ", ";
    //     }
    // }

    // std::cout << std::endl;
    // std::cout << std::endl;
    // std::cout << std::endl;
    return cost;
}

PYBIND11_MODULE(alignment, m)
{
    m.doc() = "Alignment module using pybind11";

    py::class_<AlignmentWrapper>(m, "AlignmentWrapper")
        .def(py::init<>())
        .def("setTrace", &AlignmentWrapper::setTrace, "Set the trace from a vector of strings. Don't forget to set a tree first before doing this.")
        .def("loadTree", &AlignmentWrapper::loadTree, "Load a tree from a file path")
        .def("align", &AlignmentWrapper::align, "Perform alignment and return the cost");
}