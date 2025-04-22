#include "../include/rapidxml_utils.hpp"
#include "parser.h"
#include "treeAlignment.h"
#include "treeNode.h"
#include "utils.h"
#include <algorithm>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <random>
#include <span>
#include <string>
#include <unordered_map>
#include <vector>

// Type aliases for commonly used data structures
using StringVec = std::vector<std::string>;
using IntVec = std::vector<int>;

// Global variables for activity handling
std::unordered_map<std::string, int> activitiesToInt; // Maps activity names to integer IDs
std::vector<std::string> activityVector;              // Reverse mapping: integer IDs back to activity names

// Similiar to the get_variants method from the python implementation
// removes duplicate traces

IntVec convertStringTrace(const std::vector<std::string> &trace)
{
    IntVec intTrace;
    intTrace.reserve(trace.size());

    for (const auto &activity : trace)
    {
        auto it = activitiesToInt.find(activity);
        if (it != activitiesToInt.end())
        {
            intTrace.push_back(it->second);
        }
        else
        {
            std::cerr << "Activity not found in mapping: " << activity << std::endl;
        }
    }

    return intTrace;
}

std::vector<IntVec> getVariants(const std::vector<IntVec> traces)
{
    std::unordered_map<std::vector<int>, bool, SpanHash> variantsMap;

    for (const IntVec &trace : traces)
    {
        variantsMap.emplace(trace, true);
    }

    std::vector<IntVec> variants;
    variants.reserve(variantsMap.size());
    for (const auto &pair : variantsMap)
    {
        variants.push_back(pair.first);
    }

    // seed can be made random later, right now fixed.
    std::mt19937 g(12345);
    std::shuffle(variants.begin(), variants.end(), g);

    return variants;
}

/**
 * Extracts activity sequence from a single trace XML node
 *
 * @param traceNode Pointer to an XML trace node containing event data
 * @return Vector of integer activity IDs representing the trace sequence
 */
IntVec extractActivitiesFromTrace(rapidxml::xml_node<> *traceNode)
{
    IntVec sequence;

    if (!traceNode)
        return sequence;

    // Loop through all event nodes within the trace
    for (rapidxml::xml_node<> *eventNode = traceNode->first_node("event");
         eventNode;
         eventNode = eventNode->next_sibling("event"))
    {
        // Find all string attributes in the event
        for (rapidxml::xml_node<> *stringNode = eventNode->first_node("string");
             stringNode;
             stringNode = stringNode->next_sibling("string"))
        {
            rapidxml::xml_attribute<> *keyAttr = stringNode->first_attribute("key");
            rapidxml::xml_attribute<> *valueAttr = stringNode->first_attribute("value");

            // Look for the concept:name attribute which identifies the activity
            if (keyAttr && valueAttr && std::string(keyAttr->value()) == "concept:name")
            {
                auto activityName = valueAttr->value();
                // Add the activity to the mapping if not seen before, or get existing ID
                auto [it, inserted] = activitiesToInt.emplace(activityName, activitiesToInt.size());

                if (inserted)
                {
                    // Store the activity name for reverse lookup
                    activityVector.push_back(activityName);
                }

                // Add the activity ID to the sequence
                sequence.push_back(it->second);
            }
        }
    }

    return sequence;
}

// Notes on trace storage approach:
// 1. Map activity names to integers for efficient storage and comparison
// 2. After parsing, create a vector that maps integers back to activity names
// 3. parseXes returns traces as vectors of integers (activity IDs)
// 4. parsePtml uses the same activity mapping to ensure consistency
// 5. After both parsers finish, the complete activity vector is available for reference

/**
 * Parses an XES file and extracts all traces as sequences of activity IDs
 *
 * @param filePath Path to the XES file
 * @return Vector of traces, where each trace is a vector of activity IDs
 */
std::vector<IntVec> parseXes(const std::string &filePath)
{
    rapidxml::file<> xmlFile(filePath.c_str());
    rapidxml::xml_document<> doc;
    doc.parse<0>(xmlFile.data());

    // Use map to maintain order based on trace names
    std::map<std::string, IntVec> traceMap;

    // Get the log root node
    rapidxml::xml_node<> *logNode = doc.first_node("log");
    if (!logNode)
    {
        throw std::runtime_error("Error: <log> node not found in XML.");
    }

    // Process each trace in the log
    for (rapidxml::xml_node<> *traceNode = logNode->first_node("trace");
         traceNode;
         traceNode = traceNode->next_sibling("trace"))
    {
        // Find the name attribute of the trace for identification
        rapidxml::xml_node<> *nameNode = traceNode->first_node("string");
        std::string traceValue = "UNKNOWN";

        // Look for the concept:name attribute to identify the trace
        while (nameNode)
        {
            if (std::string(nameNode->first_attribute("key")->value()) == "concept:name")
            {
                traceValue = nameNode->first_attribute("value")->value();
                break;
            }
            nameNode = nameNode->next_sibling("string");
        }

        if (traceValue != "UNKNOWN")
        {
            // Extract activities and store with trace name as key
            traceMap[traceValue] = extractActivitiesFromTrace(traceNode);
        }
        else
        {
            std::cerr << "Trace in Xes-files did not have an index";
        }
    }

    // Convert map to vector of traces (order preserved)
    std::vector<IntVec> values;
    for (const auto &pair : traceMap)
    {
        values.push_back(pair.second);
    }

    // get variants
    auto variants = getVariants(values);

    return variants;
}

/**
 * Creates a tree node from an XML node
 *
 * @param nodeName Type of the node (sequence, and, xor, etc.)
 * @param sibling XML node containing node attributes
 * @return Shared pointer to the created TreeNode
 */
std::shared_ptr<TreeNode> createNode(const std::string &nodeName, rapidxml::xml_node<> *sibling)
{
    // Check for required ID attribute
    if (!sibling || !sibling->first_attribute("id"))
        throw std::runtime_error("Error: Missing 'id' attribute in node.");

    std::string nodeId = sibling->first_attribute("id")->value();

    // Map node type names to corresponding operation enums
    static const std::unordered_map<std::string, Operation> nodeTypeMap = {
        {"sequence", SEQUENCE}, {"and", PARALLEL}, {"xor", XOR}, {"xorLoop", XOR_LOOP}, {"manualTask", ACTIVITY}, {"automaticTask", SILENT_ACTIVITY}, {"redoLoop", REDO_LOOP}};

    // Check if the node type is supported
    auto it = nodeTypeMap.find(nodeName);
    if (it == nodeTypeMap.end())
        throw std::runtime_error("Error: Unsupported node type '" + nodeName + "'");

    // Handle activity nodes specially
    if (it->second == ACTIVITY)
    {
        rapidxml::xml_attribute<> *nameAttr = sibling->first_attribute("name");
        if (!nameAttr)
            throw std::runtime_error("Error: Missing 'name' attribute for activity node.");

        // Verify activity exists in our mapping
        // originally there to verify that the activity is in the activitiesToInt map
        // if (activitiesToInt.find(nameAttr->value()) != activitiesToInt.end())
        // {
        //     return std::make_shared<TreeNode>(ACTIVITY, activitiesToInt[nameAttr->value()], nodeId);
        // }
        // else
        // {
        //     throw std::runtime_error("Activity parsed in ptml tree doesn't exist in activitiesToInt map.");
        // }

        std::string activityName = nameAttr->value();
        if (activitiesToInt.find(activityName) == activitiesToInt.end())
        {
            auto [it, inserted] = activitiesToInt.emplace(nameAttr->value(), activitiesToInt.size());
            if (inserted)
            {
                // Store the activity name for reverse lookup
                activityVector.push_back(activityName);
            }
            else
            {
                std::cerr << "Error when inserting activity" << nameAttr->value() << " into activitiesToInt map." << std::endl;
            }
        }
        return std::make_shared<TreeNode>(ACTIVITY, activitiesToInt[activityName], nodeId);
    }

    // For non-activity nodes, use -1 as placeholder for activity ID
    return std::make_shared<TreeNode>(it->second, -1, nodeId);
}

/**
 * Parses a PTML file and constructs the process tree
 *
 * @param filePath Path to the PTML file
 * @return Shared pointer to the root node of the process tree
 */
std::shared_ptr<TreeNode> parsePtml(const std::string &filePath)
{
    rapidxml::file<> xmlFile(filePath.c_str());
    rapidxml::xml_document<> doc;
    doc.parse<0>(xmlFile.data());

    // Get the process tree root node
    rapidxml::xml_node<> *processTreeNode = doc.first_node("ptml")->first_node("processTree");
    if (!processTreeNode)
    {
        throw std::runtime_error("Error: <processTree> node not found.");
    }

    // Get the ID of the root naode
    rapidxml::xml_attribute<> *rootAttr = processTreeNode->first_attribute("root");
    if (!rootAttr)
    {
        throw std::runtime_error("Error: Missing 'root' attribute in <processTree>.");
    }
    std::string rootId = rootAttr->value();

    // Map for node lookup by ID
    std::unordered_map<std::string, std::shared_ptr<TreeNode>> idToNode;

    // Process all nodes in the tree
    for (rapidxml::xml_node<> *sibling = processTreeNode->first_node(); sibling; sibling = sibling->next_sibling())
    {
        std::string siblingName = sibling->name();

        if (siblingName == "parentsNode")
        {
            // Handle parent-child relationships
            rapidxml::xml_attribute<> *sourceAttr = sibling->first_attribute("sourceId");
            rapidxml::xml_attribute<> *targetAttr = sibling->first_attribute("targetId");

            if (!sourceAttr || !targetAttr)
            {
                throw std::runtime_error("Error: Missing 'sourceId' or 'targetId' in <parentsNode>.");
            }

            std::string parentId = sourceAttr->value();
            std::string childId = targetAttr->value();

            // Verify both parent and child nodes exist before linking
            if (idToNode.find(parentId) == idToNode.end())
            {
                throw std::runtime_error("Error: Parent ID not found: " + parentId);
            }
            if (idToNode.find(childId) == idToNode.end())
            {
                throw std::runtime_error("Error: Child ID not found: " + childId);
            }

            // Add child to parent
            idToNode[parentId]->addChild(idToNode[childId]);
        }
        else
        {
            // Create a new node based on its type
            std::shared_ptr<TreeNode> newNode = createNode(siblingName, sibling);

            // Ensure node IDs are unique
            if (idToNode.find(newNode->getId()) != idToNode.end())
            {
                throw std::runtime_error("Error: Duplicate node ID found: " + newNode->getId());
            }

            // Store node in map for lookup
            idToNode[newNode->getId()] = newNode;
        }
    }

    // Verify root node exists
    if (idToNode.find(rootId) == idToNode.end())
    {
        throw std::runtime_error("Error: Root node ID not found: " + rootId);
    }

    // Initialize activity maps for the tree
    idToNode[rootId]->fillActivityMaps();

    return idToNode[rootId]; // Return the root node
}

/**
 * Generates PTML file names with different suffixes
 *
 * @param baseName Base name for the PTML file
 * @param fileEndings Vector of suffixes to append
 * @return Vector of complete PTML file names
 */
StringVec generatePtmlNames(const std::string &baseName, const StringVec &fileEndings)
{
    StringVec ptmlNames;
    for (const auto &ending : fileEndings)
    {
        ptmlNames.push_back(baseName + ending + ".ptml");
    }
    return ptmlNames;
}

/**
 * Main processing function: parses XES and PTML files and computes alignments
 *
 * @param xesPath Directory containing XES files
 * @param ptmlPath Directory containing PTML files
 */
void parseAndAlign(const std::string &xesPath, const std::string &ptmlPath)
{
    StringVec fileNames;

    // Collect all file names in the XES directory
    try
    {
        for (const auto &entry : std::filesystem::directory_iterator(xesPath))
        {
            if (entry.is_regular_file())
            {
                fileNames.push_back(entry.path().stem().string());
            }
        }
    }
    catch (const std::filesystem::filesystem_error &e)
    {
        std::cerr << "Error: " << e.what() << '\n';
        return;
    }

    // Define standard file suffixes for PTML variants
    StringVec fileEndings = {"_pt00", "_pt10", "_pt25", "_pt50"};

    // Process each file
    for (const auto &fileName : fileNames)
    {
        // Parse XES file to get traces
        const std::string fileXesPath = xesPath + fileName + ".xes";
        auto traces = parseXes(fileXesPath);

        // Determine which PTML files to use
        bool basePtmlExists = std::filesystem::exists(ptmlPath + fileName + ".ptml");
        StringVec ptmlFiles = basePtmlExists ? StringVec{fileName + ".ptml"}
                                             : generatePtmlNames(fileName, fileEndings);

        // Process each PTML file
        std::filesystem::path baseDir = std::filesystem::path(PROJECT_OUTPUT_DIR) / timeInMs();
        for (const auto &ptmlFile : ptmlFiles)
        {
            const std::string ptmlFilePath = ptmlPath + ptmlFile;
            if (!std::filesystem::exists(ptmlFilePath))
            {
                std::cerr << "File: " << ptmlFilePath << " doesn't exist.\n";
                continue;
            }
            // Parse PTML file to get process tree
            auto processTree = parsePtml(ptmlFilePath);

            // create directory for output
            std::filesystem::path directory = baseDir / ptmlFile;
            std::filesystem::create_directories(directory);

            // Open output file in append mode
            std::filesystem::path costOutputFilePath = directory / "costs.csv";
            std::ofstream costFile(costOutputFilePath, std::ios::app);
            if (!costFile)
            {
                std::cerr << "Error: Could not create or open the file!" << std::endl;
                return;
            }

            std::filesystem::path timesOutputFilePath = directory / "times.csv";
            std::ofstream timeFile(timesOutputFilePath, std::ios::app);
            if (!timeFile)
            {
                std::cerr << "Error: Could not create or open the file!" << std::endl;
                return;
            }

            // Compute alignment cost for each trace
            for (const auto &trace : traces)
            {
                costTable.clear(); // Clear cost table for new alignment
                std::span<const int> traceSpan = trace;

                auto executionStart = std::chrono::high_resolution_clock::now();
                auto cost = dynAlign(processTree, traceSpan);
                auto executionEnd = std::chrono::high_resolution_clock::now();

                // in miliseconds
                auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(executionEnd - executionStart).count();

                timeFile << duration << ", ";
                timeFile << visualizeIntTrace(trace) << ", ";
                timeFile << std::endl;

                costFile << cost << ", ";
                costFile << visualizeIntTrace(trace) << ", ";
                costFile << std::endl;
            }
        }
    }
}