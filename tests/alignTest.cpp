#define CATCH_CONFIG_MAIN

#include <catch2/catch_all.hpp>
#include "../src/utils.h"
#include "../src/treeNode.h"
#include "../src/treeAlignment.h"
#include <iostream>

// Global type aliases for better readability
using StringVec = std::vector<std::string>;
using Trace = std::shared_ptr<StringVec>;

Trace convertToTraceFormat(const std::string &str)
{
    auto vec = std::make_shared<StringVec>();
    for (char ch : str)
    {
        vec->push_back(std::string(1, ch));
    }
    return vec;
}

void testPossibleSplits(const std::shared_ptr<TreeNode> &root, const Trace trace, const std::vector<std::vector<int>> &expected)
{
    auto splits = generateSplits(root, trace);
    std::sort(splits.begin(), splits.end());

    auto sortedExpected = expected;
    std::sort(sortedExpected.begin(), sortedExpected.end());

    REQUIRE(splits == sortedExpected);
}

bool compareNestedVectors(const std::vector<std::shared_ptr<StringVec>> &vec1, const std::vector<std::shared_ptr<StringVec>> &vec2)
{
    if (vec1.size() != vec2.size())
    {
        return false;
    }

    for (size_t i = 0; i < vec1.size(); ++i)
    {
        if (!vec1[i] || !vec2[i] || *vec1[i] != *vec2[i])
        {
            return false;
        }
    }
    return true;
}

TEST_CASE("generateSplits works correctly")
{
    SECTION("Single Child")
    {
        auto root = constructTree({{PARALLEL, {std::make_shared<TreeNode>(ACTIVITY, "a"), std::make_shared<TreeNode>(ACTIVITY, "b")}}});

        SECTION("Trace: abbbabbbbabbaba")
        {
            testPossibleSplits(root, convertToTraceFormat("abbbabbbbabbaba"), {{14}});
        }

        SECTION("Empty Trace")
        {
            testPossibleSplits(root, convertToTraceFormat(""), {{-1}});
        }
    }

    SECTION("Two Children")
    {
        auto root = constructTree({{PARALLEL, {std::make_shared<TreeNode>(ACTIVITY, "a"), std::make_shared<TreeNode>(ACTIVITY, "b")}},
                                   {XOR, {std::make_shared<TreeNode>(ACTIVITY, "c"), std::make_shared<TreeNode>(ACTIVITY, "d")}}});

        SECTION("Trace: abdc")
        {
            testPossibleSplits(root, convertToTraceFormat("abdc"), {{-1, 3}, {1, 3}});
        }

        SECTION("Trace: aaaaadddaadddddddd")
        {
            testPossibleSplits(root, convertToTraceFormat("aaaaadddaadddddddd"), {{-1, 17}, {4, 17}, {9, 17}});
        }
    }

    SECTION("Three Children")
    {
        auto root = constructTree({{PARALLEL, {std::make_shared<TreeNode>(ACTIVITY, "a"), std::make_shared<TreeNode>(ACTIVITY, "b"), std::make_shared<TreeNode>(ACTIVITY, "e")}},
                                   {XOR, {std::make_shared<TreeNode>(ACTIVITY, "c"), std::make_shared<TreeNode>(ACTIVITY, "d")}},
                                   {REDO_LOOP, {std::make_shared<TreeNode>(ACTIVITY, "f"), std::make_shared<TreeNode>(ACTIVITY, "g")}}});

        SECTION("Trace: dbcda")
        {
            testPossibleSplits(root, convertToTraceFormat("dbcda"),
                               {{-1, -1, 4}, {-1, 0, 4}, {-1, 3, 4}, {1, 1, 4}, {1, 3, 4}, {4, 4, 4}});
        }

        SECTION("Trace: dbcdaf")
        {
            testPossibleSplits(root, convertToTraceFormat("dbcdaf"),
                               {{-1, -1, 5}, {-1, 0, 5}, {-1, 3, 5}, {1, 1, 5}, {1, 3, 5}, {4, 4, 5}});
        }
    }
}

TEST_CASE("segmentTrace works correctly")
{
    SECTION("Empty Trace")
    {
        auto segmentedTrace = segmentTrace(convertToTraceFormat(""), {-1});

        std::vector<Trace> expected = {std::make_shared<StringVec>()};

        REQUIRE(compareNestedVectors(segmentedTrace, expected));
    }

    SECTION("Trace: abcdcedffg")
    {
        auto segmentedTrace = segmentTrace(convertToTraceFormat("abcdcedffg"), {0, 3, 6, 9});

        std::vector<Trace> expected = {
            std::make_shared<StringVec>(StringVec{"a"}),
            std::make_shared<StringVec>(StringVec{"b", "c", "d"}),
            std::make_shared<StringVec>(StringVec{"c", "e", "d"}),
            std::make_shared<StringVec>(StringVec{"f", "f", "g"})};

        REQUIRE(compareNestedVectors(segmentedTrace, expected));

        SECTION("Mid Empty Split")
        {
            auto segmentedTrace = segmentTrace(convertToTraceFormat("abcdcedffg"), {0, 3, 3, 9});

            std::vector<Trace> expected = {
                std::make_shared<StringVec>(StringVec{"a"}),
                std::make_shared<StringVec>(StringVec{"b", "c", "d"}),
                std::make_shared<StringVec>(StringVec()),
                std::make_shared<StringVec>(StringVec{"c", "e", "d", "f", "f", "g"})};

            REQUIRE(compareNestedVectors(segmentedTrace, expected));
        }

        SECTION("Start Empty Split")
        {
            auto segmentedTrace = segmentTrace(convertToTraceFormat("abcdcedffg"), {-1, -1, 2, 9});

            std::vector<Trace> expected = {
                std::make_shared<StringVec>(),
                std::make_shared<StringVec>(),
                std::make_shared<StringVec>(StringVec{"a", "b", "c"}),
                std::make_shared<StringVec>(StringVec{"d", "c", "e", "d", "f", "f", "g"})};

            REQUIRE(compareNestedVectors(segmentedTrace, expected));
        }
    }
}

TEST_CASE("dynAlign works correctly")
{
    auto root = constructTree({{PARALLEL, {std::make_shared<TreeNode>(ACTIVITY, "a"), std::make_shared<TreeNode>(ACTIVITY, "b"), std::make_shared<TreeNode>(ACTIVITY, "e")}},
                               {XOR, {std::make_shared<TreeNode>(ACTIVITY, "c"), std::make_shared<TreeNode>(ACTIVITY, "d")}}});

    root->fillActivityMaps();

    SECTION("Empty Trace")
    {
        REQUIRE(dynAlign(root, convertToTraceFormat("")) == 4);
    }

    SECTION("Trace: eba")
    {
        REQUIRE(dynAlign(root, convertToTraceFormat("eba")) == 1);
    }

    SECTION("Trace: ebad")
    {
        REQUIRE(dynAlign(root, convertToTraceFormat("ebad")) == 0);
    }

    SECTION("Trace: babebbdddcbb")
    {
        REQUIRE(dynAlign(root, convertToTraceFormat("babebbdddcbb")) == 8);
    }

    SECTION("Loop Case")
    {
        auto sequenceNode = std::make_shared<TreeNode>(SEQUENCE);
        sequenceNode->addChild(std::make_shared<TreeNode>(ACTIVITY, "a"));
        sequenceNode->addChild(std::make_shared<TreeNode>(ACTIVITY, "b"));

        auto loopRoot = constructTree({{REDO_LOOP, {sequenceNode, std::make_shared<TreeNode>(ACTIVITY, "f")}}});

        loopRoot->fillActivityMaps();

        SECTION("Trace: abfabfabfabfabfabfabfabf")
        {
            REQUIRE(dynAlign(loopRoot, convertToTraceFormat("abfabfabfabfabfabfabfabf")) == 0);
        }

        SECTION("Trace: abbbbf")
        {
            REQUIRE(dynAlign(loopRoot, convertToTraceFormat("abbbbf")) == 2);
        }

        SECTION("Empty Trace")
        {
            REQUIRE(dynAlign(loopRoot, convertToTraceFormat("")) == 2);
        }
    }
}
