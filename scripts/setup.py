from setuptools import setup, Extension
from pybind11.setup_helpers import Pybind11Extension, build_ext
import os

# Use C++20 standard to match CMake configuration
extra_compile_args = ["-std=c++20"]

# Get the directory where setup.py is located
SETUP_DIR = os.path.dirname(os.path.abspath(__file__))

# Fix path calculation - assuming setup.py is in the "scripts" folder
PROJECT_ROOT = os.path.abspath(os.path.join(SETUP_DIR, ".."))
PROJECT_OUTPUT_DIR = os.path.join(PROJECT_ROOT, "output")

# Define source files with correct relative paths
source_files = [
    os.path.join(PROJECT_ROOT, "src/bindings.cpp"),
    os.path.join(PROJECT_ROOT, "src/treeNode.cpp"),
    os.path.join(PROJECT_ROOT, "src/treeAlignment.cpp"),
    os.path.join(PROJECT_ROOT, "src/utils.cpp"),
    os.path.join(PROJECT_ROOT, "src/parser.cpp"),
]

# Define include directories
include_dirs = [
    os.path.join(PROJECT_ROOT, "include"),
    os.path.join(PROJECT_ROOT, "src"),
]

# Add definitions to match CMake
define_macros = [
    ("PROJECT_SOURCE_DIR", f'"{PROJECT_ROOT}"'),
    ("PROJECT_OUTPUT_DIR", f'"{PROJECT_OUTPUT_DIR}"'),
    ("ENABLE_UPPER_BOUND", 1),
    ("DFS_LOOP", 1),
    ("SEQUENCE_IMPROVEMENT", 1),
    ("TRACE_PRUNING", 1),
    ("TEMP_SEQUENCE_STORING", 0)
]

ext_modules = [
    Pybind11Extension(
        "alignment",
        sources=source_files,
        include_dirs=include_dirs,
        extra_compile_args=extra_compile_args,
        define_macros=define_macros,
    ),
]

setup(
    name="alignment",
    version="0.1.0",
    author="Your Name",
    author_email="your.email@example.com",
    description="Python bindings for tree alignment",
    ext_modules=ext_modules,
    cmdclass={"build_ext": build_ext},
    zip_safe=False,
)
