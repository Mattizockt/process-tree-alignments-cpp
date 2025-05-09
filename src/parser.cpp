#include "../external/rapidxml_utils.hpp"
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
std::unordered_map<int, std::string> idToActivity;

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

void skipWhitespace(const std::string &s, size_t &pos)
{
    while (pos < s.length() && std::isspace(s[pos]))
    {
        pos++;
    }
}

// Helper function to match and consume a target string
bool matchString(const std::string &s, size_t &pos, const std::string &target)
{
    skipWhitespace(s, pos);
    if (pos + target.length() <= s.length() && s.substr(pos, target.length()) == target)
    {
        pos += target.length();
        return true;
    }
    return false;
}

std::string parseQuotedString(const std::string &s, size_t &pos)
{
    skipWhitespace(s, pos);
    if (pos >= s.length() || s[pos] != '\'')
    {
        throw std::runtime_error("Parse error at position " + std::to_string(pos) + ": Expected single quote \'.");
    }
    pos++; // Consume opening quote

    size_t start = pos;
    while (pos < s.length() && s[pos] != '\'')
    {
        // Basic implementation: assumes no escaped quotes within the name
        pos++;
    }

    if (pos >= s.length() || s[pos] != '\'')
    {
        throw std::runtime_error("Parse error: Unterminated quoted string starting at position " + std::to_string(start - 1) + ".");
    }

    std::string value = s.substr(start, pos - start);
    pos++; // Consume closing quote
    return value;
}

std::shared_ptr<TreeNode> parseNode(const std::string &treeString, size_t &pos)
{
    skipWhitespace(treeString, pos);

    if (pos >= treeString.length())
    {
        throw std::runtime_error("Parse error at position " + std::to_string(pos) + ": Unexpected end of string.");
    }

    if (treeString[pos] == '\'')
    {
        std::string activityName = parseQuotedString(treeString, pos);
        auto newNode = std::make_shared<TreeNode>(ACTIVITY);
        idToActivity[newNode->getId()] = activityName;
        activitiesToInt[activityName] = newNode->getId();
        return newNode;
    }

    if (matchString(treeString, pos, "tau"))
    {
        return std::make_shared<TreeNode>(SILENT_ACTIVITY);
    }

    Operation operation;
    if (matchString(treeString, pos, "->"))
    {
        operation = SEQUENCE;
    }
    else if (matchString(treeString, pos, "+"))
    {
        operation = PARALLEL;
    }
    else if (matchString(treeString, pos, "*"))
    {
        operation = REDO_LOOP;
    }
    else if (matchString(treeString, pos, "X"))
    {
        operation = XOR;
    }
    else
    {
        throw std::runtime_error("Parse error at position " + std::to_string(pos) + ": Unexpected token or operator '" + treeString.substr(pos, std::min(treeString.length() - pos, (size_t)10)) + "...'.");
    }

    skipWhitespace(treeString, pos);
    if (pos >= treeString.length() || treeString[pos] != '(')
    {
        throw std::runtime_error("Parse error at position " + std::to_string(pos) + ": Expected opening parenthesis '(' after operator '" + std::to_string(static_cast<int>(operation)) + "'.");
    }
    pos++; // Consume '('

    std::vector<std::shared_ptr<TreeNode>> children;
    skipWhitespace(treeString, pos);

    while (pos < treeString.length() && treeString[pos] != ')')
    {
        children.push_back(parseNode(treeString, pos)); // Recursive call to parse child

        skipWhitespace(treeString, pos);

        // Expect comma ',' if there are more children, or ')' if this is the last child
        if (pos < treeString.length() && treeString[pos] == ',')
        {
            pos++; // Consume comma
        }
        else if (pos < treeString.length() && treeString[pos] == ')')
        {
            // Ending children list
            break;
        }
        else
        {
            throw std::runtime_error("Parse error at position " + std::to_string(pos) + ": Expected ',' or ')' after child node.");
        }

        skipWhitespace(treeString, pos); // Skip space after comma or before ')'
    }

    // Expect closing parenthesis
    if (pos >= treeString.length() || treeString[pos] != ')')
    {
        throw std::runtime_error("Parse error at position " + std::to_string(pos) + ": Expected closing parenthesis ')' for operator '" + std::to_string(static_cast<int>(operation)) + "'.");
    }
    pos++; // Consume ')'

    // Create the appropriate node based on operatorType
    std::shared_ptr<TreeNode> node = std::make_shared<TreeNode>(operation);

    if (operation == REDO_LOOP && children.size() != 2) {
        throw std::runtime_error("Loop node does not exactly have 2 children");
    }
    
    for (const auto &child : children)
    {
        node->addChild(child);
    }
    
    return node;
}

std::shared_ptr<TreeNode> parseProcessTreeString(const std::string& treeString) {
    size_t pos = 0;
    std::shared_ptr<TreeNode> root = parseNode(treeString, pos);

    skipWhitespace(treeString, pos);
    if (pos < treeString.length()) {
         throw std::runtime_error("Parse error at position " + std::to_string(pos) + ": Parsing finished before end of string. Remaining input: '" + treeString.substr(pos, std::min(treeString.length() - pos, (size_t)20)) + "...'.");
    }

    // After successfully parsing, initialize activity maps for the entire tree
    if (root) {
        root->fillActivityMaps(); // Assuming this method exists and traverses the tree
    }

    std::cout << std::endl;

    return root; // Return the root node
}