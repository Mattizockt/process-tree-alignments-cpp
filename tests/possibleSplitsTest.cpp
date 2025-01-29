#define CATCH_CONFIG_MAIN
#include <catch2/catch_all.hpp>
#include "../src/utils.h"
#include "../src/treeNode.h"
#include "../src/treeAlignment.h"
#include <iostream>

// **Utility function to test possible splits**
void testPossibleSplits(
    const std::shared_ptr<TreeNode> &root,
    const std::string &trace,
    const std::vector<std::vector<int>> &expected)
{
    auto splits = generateSplits(root, trace);
    std::sort(splits.begin(), splits.end());

    auto sortedExpected = expected;
    std::sort(sortedExpected.begin(), sortedExpected.end());

    REQUIRE(splits == sortedExpected);
}

// **Tests for `generateSplits`**
TEST_CASE("generateSplits works correctly")
{

    SECTION("Single Child")
    {
        auto root = constructTree({{PARALLEL, {std::make_shared<TreeNode>(ACTIVITY, "a"), std::make_shared<TreeNode>(ACTIVITY, "b")}}});

        SECTION("Trace: abbbabbbbabbaba")
        {
            testPossibleSplits(root, "abbbabbbbabbaba", {{14}});
        }

        SECTION("Empty Trace")
        {
            testPossibleSplits(root, "", {{-1}});
        }
    }

    SECTION("Two Children")
    {
        auto root = constructTree({{PARALLEL, {std::make_shared<TreeNode>(ACTIVITY, "a"), std::make_shared<TreeNode>(ACTIVITY, "b")}},
                                   {XOR, {std::make_shared<TreeNode>(ACTIVITY, "c"), std::make_shared<TreeNode>(ACTIVITY, "d")}}});

        SECTION("Trace: abdc")
        {
            testPossibleSplits(root, "abdc", {{-1, 3}, {1, 3}});
        }

        SECTION("Trace: aaaaadddaadddddddd")
        {
            testPossibleSplits(root, "aaaaadddaadddddddd", {{-1, 17}, {4, 17}, {9, 17}});
        }
    }

    SECTION("Three Children")
    {
        auto root = constructTree({{PARALLEL, {std::make_shared<TreeNode>(ACTIVITY, "a"), std::make_shared<TreeNode>(ACTIVITY, "b"), std::make_shared<TreeNode>(ACTIVITY, "e")}},
                                   {XOR, {std::make_shared<TreeNode>(ACTIVITY, "c"), std::make_shared<TreeNode>(ACTIVITY, "d")}},
                                   {REDO_LOOP, {std::make_shared<TreeNode>(ACTIVITY, "f"), std::make_shared<TreeNode>(ACTIVITY, "g")}}});

        SECTION("Trace: dbcda")
        {
            testPossibleSplits(root, "dbcda",
                               {{-1, -1, 4}, {-1, 0, 4}, {-1, 3, 4}, {1, 1, 4}, {1, 3, 4}, {4, 4, 4}});
        }

        SECTION("Trace: dbcdaf")
        {
            testPossibleSplits(root, "dbcdaf",
                               {{-1, -1, 5}, {-1, 0, 5}, {-1, 3, 5}, {1, 1, 5}, {1, 3, 5}, {4, 4, 5}});
        }
    }
}

// **Tests for `segmentTrace`**  I I
TEST_CASE("segmentTrace works correctly")
{
    SECTION("Empty Trace")
    {
        REQUIRE(segmentTrace("", {-1}) == std::vector<std::string>{""});
    }

    SECTION("Trace: abcdcedffg")
    {
        std::string trace = "abcdcedffg";

        SECTION("Normal Split")
        {
            REQUIRE(segmentTrace(trace, {0, 3, 6, 9}) == std::vector<std::string>{"a", "bcd", "ced", "ffg"});
        }

        SECTION("Mid Empty Split")
        {
            REQUIRE(segmentTrace(trace, {0, 3, 3, 9}) == std::vector<std::string>{"a", "bcd", "", "cedffg"});
        }

        SECTION("Start Empty Split")
        {
            REQUIRE(segmentTrace(trace, {-1, -1, 2, 9}) == std::vector<std::string>{"", "", "abc", "dcedffg"});
        }
    }
}

// **Tests for `dynAlign`**
TEST_CASE("dynAlign works correctly")
{
    auto root = constructTree({{PARALLEL, {std::make_shared<TreeNode>(ACTIVITY, "a"), std::make_shared<TreeNode>(ACTIVITY, "b"), std::make_shared<TreeNode>(ACTIVITY, "e")}},
                               {XOR, {std::make_shared<TreeNode>(ACTIVITY, "c"), std::make_shared<TreeNode>(ACTIVITY, "d")}}}); // <-- Only one closing brace needed

    root->fillLetterMaps();

    SECTION("Empty Trace")
    {
        REQUIRE(dynAlign(root, "") == 4);
    }

    SECTION("Trace: eba")
    {
        REQUIRE(dynAlign(root, "eba") == 1);
    }

    SECTION("Trace: ebad")
    {
        REQUIRE(dynAlign(root, "ebad") == 0);
    }

    SECTION("Trace: babebbdddcbb")
    {
        REQUIRE(dynAlign(root, "babebbdddcbb") == 8);
    }

    SECTION("Loop Case")
    {
        // Create SEQUENCE node separately
        auto sequenceNode = std::make_shared<TreeNode>(SEQUENCE);
        sequenceNode->addChild(std::make_shared<TreeNode>(ACTIVITY, "a"));
        sequenceNode->addChild(std::make_shared<TreeNode>(ACTIVITY, "b"));

        // Now use constructTree correctly
        auto loopRoot = constructTree({{REDO_LOOP, {sequenceNode, std::make_shared<TreeNode>(ACTIVITY, "f")}}});

        loopRoot->fillLetterMaps();

        SECTION("Trace: abfabfabfabfabfabfabfabf")
        {
            REQUIRE(dynAlign(loopRoot, "abfabfabfabfabfabfabfabf") == 0);
        }

        SECTION("Trace: abababababababab123123abababababf")
        {
            REQUIRE(dynAlign(loopRoot, "abababababababab123123abababababf") == 17);
        }

        SECTION("Trace: abbbbf")
        {
            REQUIRE(dynAlign(loopRoot, "abbbbf") == 2);
        }

        SECTION("Empty Trace")
        {
            REQUIRE(dynAlign(loopRoot, "") == 2);
        }
    }
}
