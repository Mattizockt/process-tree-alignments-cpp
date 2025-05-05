#include "bindings.h"
#include "parser.h"
#include "treeAlignment.h"
#include "utils.h"
#include <iostream>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <span>
#include <string>

#include <iomanip>

class HashCollisionAnalyzer
{
private:
    std::map<size_t, int> bucketSizes; // Maps hash value to count of elements
    size_t totalItems = 0;

public:
    void analyzeHash(const std::vector<int> &v)
    {
        SpanHash hasher;
        size_t hash = hasher(v);
        bucketSizes[hash]++;
        totalItems++;
    }

    void printSummary()
    {
        size_t uniqueHashes = bucketSizes.size();
        size_t totalCollisions = totalItems - uniqueHashes;

        // Find maximum collision count
        size_t maxCollisions = 0;
        for (const auto &entry : bucketSizes)
        {
            maxCollisions = std::max(maxCollisions, (size_t)entry.second);
        }

        // Calculate collision distribution statistics
        std::map<int, int> collisionHistogram; // Maps bucket size to frequency
        double sumSquaredDeviations = 0.0;

        for (const auto &entry : bucketSizes)
        {
            int bucketSize = entry.second;
            collisionHistogram[bucketSize]++;

            // For standard deviation calculation
            double deviation = bucketSize - 1.0; // Ideal is 1 element per bucket
            sumSquaredDeviations += deviation * deviation;
        }

        // Calculate statistics
        double collisionRate = uniqueHashes > 0 ? static_cast<double>(totalCollisions) / totalItems * 100.0 : 0.0;
        double avgElementsPerBucket = uniqueHashes > 0 ? static_cast<double>(totalItems) / uniqueHashes : 0.0;
        double stdDeviation = uniqueHashes > 0 ? std::sqrt(sumSquaredDeviations / uniqueHashes) : 0.0;

        // Print ONLY summary statistics - NO individual buckets
        std::cout << "===== Hash Collision Summary =====" << std::endl;
        std::cout << "Total elements: " << totalItems << std::endl;
        std::cout << "Unique hash values: " << uniqueHashes << std::endl;
        std::cout << "Total collisions: " << totalCollisions << std::endl;
        std::cout << "Collision rate: " << std::fixed << std::setprecision(2) << collisionRate << "%" << std::endl;
        std::cout << "Average elements per bucket: " << std::fixed << std::setprecision(2) << avgElementsPerBucket << std::endl;
        std::cout << "Standard deviation of bucket sizes: " << std::fixed << std::setprecision(2) << stdDeviation << std::endl;
        std::cout << "Maximum elements in a single bucket: " << maxCollisions << std::endl;

        // Print distribution statistics (NOT individual buckets)
        std::cout << "\nCollision Distribution Summary:" << std::endl;
        std::cout << "Elements per bucket | Number of buckets | % of elements" << std::endl;
        std::cout << "--------------------|------------------|-------------" << std::endl;

        for (const auto &entry : collisionHistogram)
        {
            int elementsPerBucket = entry.first;
            int numBuckets = entry.second;
            double percentElements = static_cast<double>(elementsPerBucket * numBuckets) / totalItems * 100.0;

            std::cout << std::setw(19) << elementsPerBucket << " | ";
            std::cout << std::setw(17) << numBuckets << " | ";
            std::cout << std::fixed << std::setprecision(2) << std::setw(11) << percentElements << "%" << std::endl;
        }
    }

    // Analyze an entire unordered_map with timing information
    void analyzeMap(const std::unordered_map<std::string,
                                             std::unordered_map<std::vector<int>, int, SpanHash, SpanEqual>> &costTable)
    {
        // auto start = std::chrono::high_resolution_clock::now();

        for (const auto &outer : costTable)
        {
            for (const auto &inner : outer.second)
            {
                analyzeHash(inner.first);
            }
        }

        // auto end = std::chrono::high_resolution_clock::now();
        // std::chrono::duration<double> elapsed = end - start;

        printSummary();
        // std::cout << "\nAnalysis completed in " << elapsed.count() << " seconds" << std::endl;
    }
};

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

    HashCollisionAnalyzer analyzer;
    analyzer.analyzeMap(costTable);
    // analyzer.printStatistics();

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