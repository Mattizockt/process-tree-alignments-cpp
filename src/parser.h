#ifndef PARSER_H
#define PARSER_H
#include <string>
#include <memory>
#include "treeNode.h"

std::vector<std::vector<std::string>> parseXes(const std::string &filePath);
std::shared_ptr<TreeNode> parsePtml(const std::string &filePath);
void parseAndAlign(const std::string &xesPath, const std::string &ptmlPath, const std::string &outputFileName);
#endif // PARSER_H