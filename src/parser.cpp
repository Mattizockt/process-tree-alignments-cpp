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

/**
 * Global dictionaries for activity handling
 * These maps provide bidirectional mapping between activity names and their integer IDs
 */
std::unordered_map<std::string, int> activitiesToInt; // Maps activity names to integer IDs
std::unordered_map<int, std::string> idToActivity;    // Maps integer IDs back to activity names
std::unordered_map<int, std::shared_ptr<TreeNode>> tempNodeMap;

/**
 * Advances the position pointer past any whitespace characters
 *
 * @param s The input string being parsed
 * @param pos Position reference that will be updated
 */
void skipWhitespace(const std::string &s, size_t &pos)
{
    while (pos < s.length() && std::isspace(s[pos]))
    {
        pos++;
    }
}

/**
 * Tries to match a specific string at the current position
 *
 * @param s The input string being parsed
 * @param pos Position reference that will be updated if match succeeds
 * @param target The string to match
 * @return true if match succeeds, false otherwise
 */
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

/**
 * Parses a single-quoted string from the input
 *
 * @param s The input string being parsed
 * @param pos Position reference that will be updated
 * @return The parsed string without quotes
 * @throws std::runtime_error if quoted string is malformed
 */
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

/**
 * Recursively parses a tree node from the input string
 *
 * @param treeString The input string representing the process tree
 * @param pos Position reference that will be updated
 * @return A shared pointer to the parsed TreeNode
 * @throws std::runtime_error for various parsing errors
 */
std::shared_ptr<TreeNode> parseNode(const std::string &treeString, size_t &pos)
{
    skipWhitespace(treeString, pos);

    if (pos >= treeString.length())
    {
        throw std::runtime_error("Parse error at position " + std::to_string(pos) + ": Unexpected end of string.");
    }

    // Case 1: Activity node (quoted string)
    if (treeString[pos] == '\'')
    {
        std::string activityName = parseQuotedString(treeString, pos);
        auto newNode = std::make_shared<TreeNode>(ACTIVITY);
        idToActivity[newNode->getId()] = activityName;
        activitiesToInt[activityName] = newNode->getId();
        return newNode;
    }

    // Case 2: Silent activity (tau)
    if (matchString(treeString, pos, "tau"))
    {
        return std::make_shared<TreeNode>(SILENT_ACTIVITY);
    }

    // Case 3: Operator node
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
        throw std::runtime_error("Parse error at position " + std::to_string(pos) +
                                 ": Unexpected token or operator '" +
                                 treeString.substr(pos, std::min(treeString.length() - pos, (size_t)10)) + "...'.");
    }

    // Parse operator's children within parentheses
    skipWhitespace(treeString, pos);
    if (pos >= treeString.length() || treeString[pos] != '(')
    {
        throw std::runtime_error("Parse error at position " + std::to_string(pos) +
                                 ": Expected opening parenthesis '(' after operator '" +
                                 std::to_string(static_cast<int>(operation)) + "'.");
    }
    pos++; // Consume '('

    // Parse all children nodes
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
            throw std::runtime_error("Parse error at position " + std::to_string(pos) +
                                     ": Expected ',' or ')' after child node.");
        }

        skipWhitespace(treeString, pos); // Skip space after comma or before ')'
    }

    // Expect closing parenthesis
    if (pos >= treeString.length() || treeString[pos] != ')')
    {
        throw std::runtime_error("Parse error at position " + std::to_string(pos) +
                                 ": Expected closing parenthesis ')' for operator '" +
                                 std::to_string(static_cast<int>(operation)) + "'.");
    }
    pos++; // Consume ')'

    // Create the appropriate node based on operation type
    std::shared_ptr<TreeNode> node = std::make_shared<TreeNode>(operation);

    // Add all children to the node
    for (const auto &child : children)
    {
        node->addChild(child);
    }

    // Validate redo loop has exactly 2 children
    if (operation == REDO_LOOP)
    {
        if (children.size() != 2)
        {
            throw std::runtime_error("Loop node does not exactly have 2 children");
        }
#if TEMP_SEQUENCE_STORING == 1
        else
        {
            const int tempNodeId = node->getId() * -1;
            std::shared_ptr<TreeNode> tempNode = std::make_shared<TreeNode>(SEQUENCE, tempNodeId);
            tempNodeMap[tempNodeId] = tempNode;
            tempNode->addChild(children[1]);
            tempNode->addChild(children[0]);
#if TRACE_PRUNING == 1
            std::unordered_set<int> allActivities;
            allActivities.reserve(children[0]->getActivities().size() + children[1]->getActivities().size());

            std::set_union(
                children[0]->getActivities().begin(), children[0]->getActivities().end(),
                children[1]->getActivities().begin(), children[1]->getActivities().end(),
                std::inserter(allActivities, allActivities.begin()));

            tempNode->setActivities(allActivities);
#endif
        }
#endif
    }

    // not very efficient for frequent tree building but fine if align many traces with a tree
    /// TODO perhaps change this function if everything works
    node->fillActivityMaps();
    return node;
}

/**
 * Converts a trace of activity names to their corresponding integer IDs
 *
 * @param trace Vector of activity names
 * @return Vector of corresponding integer IDs
 */
std::vector<int> convertStringTrace(const std::vector<std::string> &trace)
{
    std::vector<int> intTrace;
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
            intTrace.push_back(-1);
            std::cerr << "Activity not found in mapping: " << activity << std::endl;
        }
    }

    return intTrace;
}

/**
 * Main entry point for parsing a process tree string
 *
 * @param treeString String representation of the process tree
 * @return Root node of the parsed process tree
 * @throws std::runtime_error for various parsing errors
 */
std::shared_ptr<TreeNode> parseProcessTreeString(const std::string &treeString)
{
    // Start parsing from the beginning of the string
    size_t pos = 0;
    std::shared_ptr<TreeNode> root = parseNode(treeString, pos);

    // Ensure the entire string was consumed
    skipWhitespace(treeString, pos);
    if (pos < treeString.length())
    {
        throw std::runtime_error("Parse error at position " + std::to_string(pos) +
                                 ": Parsing finished before end of string. Remaining input: '" +
                                 treeString.substr(pos, std::min(treeString.length() - pos, (size_t)20)) + "...'.");
    }

    // After successfully parsing, initialize activity maps for the entire tree
    if (root)
    {
        root->fillActivityMaps(); // Traverses the tree to build complete activity maps
    }

    return root; // Return the root node
}