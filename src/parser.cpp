#include "../include/rapidxml_utils.hpp"
#include "parser.h"
#include "utils.h"
#include "treeNode.h"
#include <iostream>
#include <string>
#include <vector>

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

Operation stringToOperation(const std::string &opStr)
{
    if (opStr == "SEQUENCE")
        return SEQUENCE;
    else if (opStr == "PARALLEL")
        return PARALLEL;
    else if (opStr == "XOR")
        return XOR;
    else if (opStr == "REDO_LOOP")
        return REDO_LOOP;
    else if (opStr == "ACTIVITY")
        return ACTIVITY;
    else if (opStr == "SILENT_ACTIVITY")
        return SILENT_ACTIVITY;
    else
        throw std::runtime_error("Operation: " + opStr + " not implemented.");
        // TODO perhaps add an unknown operation in the future
}

std::shared_ptr<TreeNode> parsePTMLNode(rapidxml::xml_node<> *xmlNode)
{
    if (!xmlNode)
    {
        return nullptr;
    }

    rapidxml::xml_attribute<> *opAttr = xmlNode->first_attribute("operator");
    rapidxml::xml_attribute<> *actAttr = xmlNode->first_attribute("activity");

    std::shared_ptr<TreeNode> node;

    if (opAttr)
    {
        // TODO doesn't work yet
        node = std::make_shared<TreeNode>(static_cast<Operation>(stringToOperation(opAttr->value())));
    }
    else if (actAttr)
    {
        // TODO what happens with silent activities
        node = std::make_shared<TreeNode>(ACTIVITY, actAttr->value());
    }
    else
    {
        // TODO throw error?
        return nullptr;
    }

    // Recursively process children
    for (rapidxml::xml_node<> *child = xmlNode->first_node("node"); child; child = child->next_sibling("node"))
    {
        std::shared_ptr<TreeNode> childNode = parsePTMLNode(child);
        if (childNode)
        {
            node->addChild(childNode);
        }
    }

    return node;
}

std::shared_ptr<TreeNode> parsePtml(const std::string &filePath)
{
    rapidxml::file<> xmlFile(filePath.c_str());
    rapidxml::xml_document<> doc;
    doc.parse<0>(xmlFile.data());

    rapidxml::xml_node<> *rootNode = doc.first_node("processTree")->first_node("node");
    std::shared_ptr<TreeNode> processTree = parsePTMLNode(rootNode);

    processTree->printTree();
    return processTree;
}