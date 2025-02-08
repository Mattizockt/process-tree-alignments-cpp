#include "../include/rapidxml_utils.hpp"
#include "../include/nlohmann/json.hpp"
#include "parser.h"
#include "utils.h"
#include "treeNode.h"
#include "treeAlignment.h"
#include <fstream>
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

    std::map<std::string, StringVec> traceMap;
    // std::vector<StringVec> traceSequences;

    rapidxml::xml_node<> *logNode = doc.first_node("log");
    if (!logNode)
    {
        throw std::runtime_error("Error: <log> node not found in XML.");
    }

    for (rapidxml::xml_node<> *traceNode = logNode->first_node("trace");
         traceNode;
         traceNode = traceNode->next_sibling("trace"))
    {
        rapidxml::xml_node<> *nameNode = traceNode->first_node("string");
        std::string traceValue = "UNKNOWN";

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
            traceMap[traceValue] = extractActivitiesFromTrace(traceNode);
        }
        else
        {
            std::cerr << "Trace in Xes-files did not have an index";
        }
        // traceSequences.emplace_back(extractActivitiesFromTrace(traceNode));
    }

    std::vector<StringVec> values;
    for (const auto &pair : traceMap)
        values.push_back(pair.second);

    return values;
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

StringVec generatePtmlNames(const std::string &baseName, const StringVec &fileEndings, const std::string &ptmlPath)
{
    std::cout << "basename: " << baseName << " ptmlpath: " << ptmlPath;
    StringVec ptmlNames;
    for (const auto &ending : fileEndings)
    {
        ptmlNames.push_back(ptmlPath + baseName + ending + ".ptml");
    }
    return ptmlNames;
}

void parseAndAlign(const std::string &xesPath, const std::string &ptmlPath, const std::string &outputFileName)
{
    StringVec fileNames;

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

    std::ofstream outFile(outputFileName);
    if (!outFile)
    {
        std::cerr << "Error: Could not create the file!" << std::endl;
        return;
    }

    nlohmann::json evalJson;
    StringVec fileEndings = {"_pt00", "_pt10", "_pt25", "_pt50"};

    for (const auto &fileName : fileNames)
    {
        const std::string fileXesPath = xesPath + fileName + ".xes";
        auto trace = parseXes(fileXesPath);

        bool basePtmlExists = std::filesystem::exists(ptmlPath + fileName + ".ptml");

        StringVec ptmlFiles = basePtmlExists ? StringVec{fileName + ".ptml"}
                                             : generatePtmlNames(fileName, fileEndings, ptmlPath);

        for (const auto &ptmlName : ptmlFiles)
        {
            if (!std::filesystem::exists(ptmlName))
            {
                std::cerr << "File: " << ptmlPath << ptmlName << " doesn't exist.\n";
                continue;
            }

            auto processTree = parsePtml(ptmlName);
            int count = 0;
            for (const auto &otherTrace : trace)
            {
                auto cost = dynAlign(processTree, std::make_shared<StringVec>(otherTrace));
                evalJson[ptmlName][std::to_string(count++)]["adv. dyn. c++"] = cost;
            }
        }
    }

    outFile << evalJson.dump(4);
}