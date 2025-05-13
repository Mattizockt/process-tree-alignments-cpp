#include "bindings.h"
#include "parser.h"
#include "treeAlignment.h"
#include "utils.h"
#include <iostream>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <span>
#include <string>
#include <atomic>
#include <thread>
#include <future>

namespace py = pybind11;

AlignmentWrapper::AlignmentWrapper()
{
}

void AlignmentWrapper::loadTree(std::string tree)
{
    processTree = parseProcessTreeString(tree);
}

size_t AlignmentWrapper::align(const std::vector<std::string> newTrace) const
{
    std::vector<int> intTrace = convertStringTrace(newTrace);
    std::span<const int> trace = intTrace;

    stop_flag.store(false);
    int timeout_seconds = 10;

    costTable.clear();
    std::future<size_t> result_future = std::async(std::launch::async, dynAlign, std::ref(processTree), std::ref(trace));
    std::future_status status = result_future.wait_for(std::chrono::seconds(timeout_seconds));

    if (status == std::future_status::ready)
    {
        return result_future.get();
    }
    else
    {
        stop_flag.store(true);
        std::cout << "error";
        return std::numeric_limits<size_t>::max();
    }
    // const size_t cost = dynAlign(processTree, trace);
    // return cost;
}

PYBIND11_MODULE(alignment, m)
{
    m.doc() = "Alignment module using pybind11";

    py::class_<AlignmentWrapper>(m, "AlignmentWrapper")
        .def(py::init<>())
        .def("loadTree", &AlignmentWrapper::loadTree, "Load a tree from a file path")
        .def("align", &AlignmentWrapper::align, "Perform alignment and return the cost");
}