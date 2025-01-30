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

std::vector<std::string> parseXes(const std::string &filePath)
{
    rapidxml::file<> xmlFile(filePath.c_str());
    rapidxml::xml_document<> doc;
    doc.parse<0>(xmlFile.data());

    std::vector<std::string> traceSequences;

    for (rapidxml::xml_node<> *traceNode = doc.first_node("log")->first_node("trace");
         traceNode;
         traceNode = traceNode->next_sibling("trace"))
    {

        std::string sequence;

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
                    sequence += valueAttr->value();
                }
            }
        }

        traceSequences.push_back(sequence);
    }

    printVector(traceSequences);
    return traceSequences;
}

std::shared_ptr<TreeNode> createNode(const std::string &nodeName, rapidxml::xml_node<> *sibling)
{
    if (!sibling || !sibling->first_attribute("id"))
    {
        throw std::runtime_error("Error: Missing 'id' attribute in node.");
    }

    std::string nodeId = sibling->first_attribute("id")->value();

    static const std::unordered_map<std::string, Operation> nodeTypeMap = {
        {"sequence", SEQUENCE},
        {"and", PARALLEL},
        {"xor", XOR},
        {"xorLoop", REDO_LOOP},
        {"manualTask", ACTIVITY},
        {"automaticTask", SILENT_ACTIVITY}};

    auto it = nodeTypeMap.find(nodeName);
    if (it != nodeTypeMap.end())
    {
        return std::make_shared<TreeNode>(it->second, "", nodeId);
    }

    throw std::runtime_error("Error: Unsupported node type '" + nodeName + "'");
}

std::shared_ptr<TreeNode> parsePtml(const std::string &filePath)
{
    rapidxml::file<> xmlFile(filePath.c_str());
    rapidxml::xml_document<> doc;
    doc.parse<0>(xmlFile.data());

    rapidxml::xml_node<> *processTreeNode = doc.first_node("ptml")->first_node("processTree")->first_node();
    if (!processTreeNode)
    {
        throw std::runtime_error("Error: <processTree> node not found.");
    }

    rapidxml::xml_node<> *node = processTreeNode->first_node();
    if (!node)
    {
        throw std::runtime_error("Error: No child nodes found in <processTree>.");
    }

    std::unordered_map<std::string, std::shared_ptr<TreeNode>> idToNode;

    for (rapidxml::xml_node<> *sibling = processTreeNode; sibling; sibling = sibling->next_sibling())
    {
        std::string siblingName = sibling->name();

        if (siblingName == "parentsNode")
        {
            // check if element exists
            rapidxml::xml_attribute<> *sourceAttr = sibling->first_attribute("sourceId");
            rapidxml::xml_attribute<> *targetAttr = sibling->first_attribute("targetId");

            if (!sourceAttr || !targetAttr)
            {
                throw std::runtime_error("Error: Missing 'sourceId' or 'targetId' in <parentsNode>.");
            }

            std::string parentId = sourceAttr->value();
            std::string childId = targetAttr->value();

            if (idToNode.find(parentId) == idToNode.end() || idToNode.find(childId) == idToNode.end())
            {
                throw std::runtime_error("Error: Invalid parent-child reference.");
            }

            idToNode[parentId]->addChild(idToNode[childId]);
        }
        else
        {
            std::shared_ptr<TreeNode> newNode = createNode(siblingName, sibling);
            idToNode[newNode->getId()] = newNode;
        }
    }

    rapidxml::xml_attribute<> *rootAttr = processTreeNode->first_attribute("root");
    if (!rootAttr)
    {
        throw std::runtime_error("Error: Missing 'root' attribute in <processTree>.");
    }

    std::string rootId = rootAttr->value();
    if (idToNode.find(rootId) == idToNode.end())
    {
        throw std::runtime_error("Error: Root node ID not found in parsed nodes.");
    }

    auto root = idToNode[rootId];
    root->printTree();
    return root;
}

// TODO use python parser to bring into the correct format
std::vector<std::string> createPtmlXesPairs()
{

    std::string xesPath = "../data/xes/"; // Relative xesPath to directory
    std::vector<std::string> fileNames;

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

    printVector(fileNames);

    std::string ptmlPath = "../data/ptml/";
    std::vector<std::string> fileEndings = {"_pt00", "_pt10", "_pt25", "_pt50"};

    for (const auto &fileName : fileNames)
    {
        const std::string fileXesPath = xesPath + fileName + ".xes";
        auto trace = parseXes(fileXesPath);
        printVector(trace);

        for (const auto &fileEnding : fileEndings)
        {
            const std::string filePtmlPath = ptmlPath + fileName + fileEnding + ".ptml";

            auto processTree = parsePtml(filePtmlPath);
            processTree->printTree();

            // change to vector of strings
            // dynAlign(processTree, trace);
        }
    }

    return fileNames;
}