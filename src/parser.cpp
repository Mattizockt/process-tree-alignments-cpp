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

    std::cout << "File size: " << xmlFile.size() << " bytes" << std::endl;

    rapidxml::xml_node<> *processTreeNode = doc.first_node("ptml")->first_node("processTree");
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
    for (rapidxml::xml_node<> *sibling = node; sibling; sibling = sibling->next_sibling())
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
    std::cout << std::endl;
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

        for (const auto &fileEnding : fileEndings)
        {
            const std::string filePtmlPath = ptmlPath + fileName + fileEnding + ".ptml";
            std::cout << filePtmlPath << std::endl;

            auto processTree = parsePtml(filePtmlPath);

            // change to vector of strings

            for (const auto &otherTrace : trace) {
                // TODO improve later
                // dynAlign(processTree, std::make_shared<std::vector<std::string>>(otherTrace));
            }
        }
    }

    return fileNames;
}