#ifndef PARSER_H
#define PARSER_H
#include "treeNode.h"
#include <atomic>
#include <memory>
#include <string>

extern std::unordered_map<std::string, int> activitiesToInt;
extern std::unordered_map<int, std::string> idToActivity;
extern std::unordered_map<int, std::shared_ptr<TreeNode>> tempNodeMap;

std::vector<int> convertStringTrace(const std::vector<std::string> &trace);
std::shared_ptr<TreeNode> parseProcessTreeString(const std::string& treeString);
#endif // PARSER_H