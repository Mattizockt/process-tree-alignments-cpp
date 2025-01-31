#include "../include/rapidxml_utils.hpp"
#include "parser.h"
#include "utils.h"
#include "treeNode.h"
#include "treeAlignment.h"
#include <iostream>
#include <string>
#include <vector>
#include <unordered_map>
#include <filesystem>

using StringVec = std::vector<std::string>;

// Extract activities from a single trace node
StringVec extractActivitiesFromTrace(rapidxml::xml_node<> *traceNode)
{
    StringVec sequence;

    if (!traceNode)
        return sequence;

    for (rapidxml::xml_node<> *eventNode = traceNode->first_node("event");
         eventNode;
         eventNode = eventNode->next_sibling("event"))
    {
        for (rapidxml::xml_node<> *stringNode = eventNode->first_node("string");
             stringNode;
             stringNode = stringNode->next_sibling("string"))
        {
            rapidxml::xml_attribute<> *keyAttr = stringNode->first_attribute("key");
            rapidxml::xml_attribute<> *valueAttr = stringNode->first_attribute("value");

            if (keyAttr && valueAttr && std::string(keyAttr->value()) == "concept:name")
            {
                sequence.emplace_back(valueAttr->value()); // Use emplace_back for efficiency
            }
        }
    }

    return sequence;
}

// Parse the XES file and extract traces
std::vector<StringVec> parseXes(const std::string &filePath)
{
    rapidxml::file<> xmlFile(filePath.c_str());
    rapidxml::xml_document<> doc;
    doc.parse<0>(xmlFile.data());

    std::vector<StringVec> traceSequences;

    rapidxml::xml_node<> *logNode = doc.first_node("log");
    if (!logNode)
    {
        throw std::runtime_error("Error: <log> node not found in XML.");
    }

    for (rapidxml::xml_node<> *traceNode = logNode->first_node("trace");
         traceNode;
         traceNode = traceNode->next_sibling("trace"))
    {
        traceSequences.emplace_back(extractActivitiesFromTrace(traceNode));
    }

    return traceSequences;
}

std::shared_ptr<TreeNode> createNode(const std::string &nodeName, rapidxml::xml_node<> *sibling)
{
    if (!sibling || !sibling->first_attribute("id"))
        throw std::runtime_error("Error: Missing 'id' attribute in node.");

    std::string nodeId = sibling->first_attribute("id")->value();

    static const std::unordered_map<std::string, Operation> nodeTypeMap = {
        {"sequence", SEQUENCE}, {"and", PARALLEL}, {"xor", XOR}, {"xorLoop", XOR_LOOP}, {"manualTask", ACTIVITY}, {"automaticTask", SILENT_ACTIVITY}, {"redoLoop", REDO_LOOP}};

    auto it = nodeTypeMap.find(nodeName);
    if (it == nodeTypeMap.end())
        throw std::runtime_error("Error: Unsupported node type '" + nodeName + "'");

    if (it->second == ACTIVITY)
    {
        rapidxml::xml_attribute<> *nameAttr = sibling->first_attribute("name");
        if (!nameAttr)
            throw std::runtime_error("Error: Missing 'name' attribute for activity node.");
        return std::make_shared<TreeNode>(ACTIVITY, nameAttr->value(), nodeId);
    }

    return std::make_shared<TreeNode>(it->second, "", nodeId);
}

std::shared_ptr<TreeNode> parsePtml(const std::string &filePath)
{
    rapidxml::file<> xmlFile(filePath.c_str());
    rapidxml::xml_document<> doc;
    doc.parse<0>(xmlFile.data());

    rapidxml::xml_node<> *processTreeNode = doc.first_node("ptml")->first_node("processTree");
    if (!processTreeNode)
    {
        throw std::runtime_error("Error: <processTree> node not found.");
    }

    rapidxml::xml_attribute<> *rootAttr = processTreeNode->first_attribute("root");
    if (!rootAttr)
    {
        throw std::runtime_error("Error: Missing 'root' attribute in <processTree>.");
    }
    std::string rootId = rootAttr->value();

    std::unordered_map<std::string, std::shared_ptr<TreeNode>> idToNode;
    std::unordered_map<std::string, bool> referencedNodes;

    for (rapidxml::xml_node<> *sibling = processTreeNode->first_node(); sibling; sibling = sibling->next_sibling())
    {
        std::string siblingName = sibling->name();

        if (siblingName == "parentsNode")
        {
            rapidxml::xml_attribute<> *sourceAttr = sibling->first_attribute("sourceId");
            rapidxml::xml_attribute<> *targetAttr = sibling->first_attribute("targetId");

            if (!sourceAttr || !targetAttr)
            {
                throw std::runtime_error("Error: Missing 'sourceId' or 'targetId' in <parentsNode>.");
            }

            std::string parentId = sourceAttr->value();
            std::string childId = targetAttr->value();

            // Ensure parent & child nodes exist before linking
            if (idToNode.find(parentId) == idToNode.end())
            {
                throw std::runtime_error("Error: Parent ID not found: " + parentId);
            }
            if (idToNode.find(childId) == idToNode.end())
            {
                throw std::runtime_error("Error: Child ID not found: " + childId);
            }

            idToNode[parentId]->addChild(idToNode[childId]);
            referencedNodes[childId] = true; // Mark child as referenced
        }
        else
        {
            // Create a new node
            std::shared_ptr<TreeNode> newNode = createNode(siblingName, sibling);

            // Ensure no duplicate IDs
            if (idToNode.find(newNode->getId()) != idToNode.end())
            {
                throw std::runtime_error("Error: Duplicate node ID found: " + newNode->getId());
            }

            idToNode[newNode->getId()] = newNode;
        }
    }

    if (idToNode.find(rootId) == idToNode.end())
    {
        throw std::runtime_error("Error: Root node ID not found: " + rootId);
    }

    for (const auto &entry : idToNode)
    {
        if (entry.first != rootId && referencedNodes.find(entry.first) == referencedNodes.end())
        {
            std::cerr << "Warning: Node ID " << entry.first << " is not linked to any parent.\n";
        }
    }

    idToNode[rootId]->fillActivityMaps();
    return idToNode[rootId]; // Return root node
}

// TODO use python parser to bring into the correct format
StringVec createPtmlXesPairs(const std::string xesPath,const std::string ptmlPath)
{
    StringVec fileNames;

    try
    {
        for (const auto &entry : std::filesystem::directory_iterator(xesPath))
        {
            if (entry.is_regular_file())
            { // Check if it's a file (not a directory)
                fileNames.push_back(entry.path().stem().string());
            }
        }
    }
    catch (const std::filesystem::filesystem_error &e)
    {
        std::cerr << "Error: " << e.what() << '\n';
        return fileNames; // Exit if directory is not found or inaccessible
    }

    StringVec fileEndings = {"_pt00", "_pt10", "_pt25", "_pt50"};

    // maybe some error checking

    for (const auto &fileName : fileNames)
    {
        const std::string fileXesPath = xesPath + fileName + ".xes";
        auto trace = parseXes(fileXesPath);

        for (const auto &fileEnding : fileEndings)
        {
            const std::string filePtmlPath = ptmlPath + fileName + fileEnding + ".ptml";
            std::cout << filePtmlPath << std::endl;

            auto processTree = parsePtml(filePtmlPath);

            // change to vector of strings

            for (const auto &otherTrace : trace)
            {
                // TODO improve later
                std::cout << "PTML Filename: " << filePtmlPath << std::endl;
                // std::cout << "This process tree: " << std::endl;
                // processTree->printTree();
                // std::cout << std::endl;
                std::cout << "And this Trace: " << std::endl;
                printVector(otherTrace);
                // std::cout << std::endl;
                std::cout << "have this cost: " << dynAlign(processTree, std::make_shared<StringVec>(otherTrace)) << std::endl;
                std::cout << std::endl;
            }
        }
    }

    return fileNames;
}