#ifndef PARSER_H
#define PARSER_H
#include <string>
#include <memory>
#include "treeNode.h"
#include <atomic>

extern std::unordered_map<std::string, int> activitiesToInt;
extern std::vector<std::string> activityVector;
extern std::atomic<bool> stopFlag;

std::vector<std::vector<int>> parseXes(const std::string &filePath);
std::shared_ptr<TreeNode> parsePtml(const std::string &filePath);
void parseAndAlign(const std::string &xesPath, const std::string &ptmlPath, const std::string &outputFileName);
#endif // PARSER_H