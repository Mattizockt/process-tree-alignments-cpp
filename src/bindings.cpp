#include <pybind11/pybind11.h>
#include "parser.h"

namespace py = pybind11;

PYBIND11_MODULE(treenode, m) {
    m.doc() = "My pybind11 example module";
    
    // Add your bindings here
    m.def("parse_and_align", &parseAndAlign, "Aligns the PTML and Trace that are specified");
    
}