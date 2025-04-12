#ifndef PARSER_H
#define PARSER_H
#include "treeNode.h"
#include <atomic>
#include <memory>
#include <string>

extern std::unordered_map<std::string, int> activitiesToInt;
extern std::vector<std::string> activityVector;

std::vector<std::vector<int>> parseXes(const std::string &filePath);
std::shared_ptr<TreeNode> parsePtml(const std::string &filePath);
void parseAndAlign(const std::string &xesPath, const std::string &ptmlPath);
#endif // PARSER_H