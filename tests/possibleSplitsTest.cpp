#define CATCH_CONFIG_MAIN
#include <catch2/catch_all.hpp>
#include "../src/utils.h"
#include "../src/treeProcessor.h"
#include <iostream>

void testPossibleSplits(const std::shared_ptr<TreeNode> &root, const std::string &trace, const std::vector<std::vector<int>> &expected)
{
    auto splits = generateSplits(root, trace);
    std::sort(splits.begin(), splits.end());
    auto sortedExpected = expected;
    std::sort(sortedExpected.begin(), sortedExpected.end());
    REQUIRE(splits == sortedExpected);
}

TEST_CASE("Does possibleSplits work?")
{

    SECTION("Single Child")
    {
        auto root = std::make_shared<TreeNode>(SEQUENCE);

        auto child1 = std::make_shared<TreeNode>(PARALLEL);

        auto child3 = std::make_shared<TreeNode>(ACTIVITY, "a");
        auto child4 = std::make_shared<TreeNode>(ACTIVITY, "b");

        child1->addChild(child3);
        child1->addChild(child4);

        root->addChild(child1);

        root->fillLetterMaps();

        SECTION("abbbabbbbabbaba")
        {
            std::string trace = "abbbabbbbabbaba";
            std::vector<std::vector<int>> expected = {{14}};
            testPossibleSplits(root, trace, expected);
        }

        SECTION("")
        {
            std::string trace = "";
            std::vector<std::vector<int>> expected = {{-1}};
            testPossibleSplits(root, trace, expected);
        }
    }

    SECTION("Two Children")
    {
        auto root = std::make_shared<TreeNode>(SEQUENCE);

        auto child1 = std::make_shared<TreeNode>(PARALLEL);
        auto child2 = std::make_shared<TreeNode>(XOR);

        auto child3 = std::make_shared<TreeNode>(ACTIVITY, "a");
        auto child4 = std::make_shared<TreeNode>(ACTIVITY, "b");
        auto child5 = std::make_shared<TreeNode>(ACTIVITY, "c");
        auto child6 = std::make_shared<TreeNode>(ACTIVITY, "d");

        child1->addChild(child3);
        child1->addChild(child4);

        child2->addChild(child5);
        child2->addChild(child6);

        root->addChild(child1);
        root->addChild(child2);

        root->fillLetterMaps();

        SECTION("abdc")
        {
            std::string trace = "abdc";
            std::vector<std::vector<int>> expected = {{-1, 3}, {1, 3}};
            testPossibleSplits(root, trace, expected);
        }
        SECTION("aaaaadddaadddddddd")
        {
            std::string trace = "aaaaadddaadddddddd";
            std::vector<std::vector<int>> expected = {{-1, 17}, {4, 17}, {9, 17}};
            testPossibleSplits(root, trace, expected);
        }
    }

    SECTION("Three Children")
    {
        auto root = std::make_shared<TreeNode>(SEQUENCE);

        auto child1 = std::make_shared<TreeNode>(PARALLEL);
        auto child2 = std::make_shared<TreeNode>(XOR);
        auto loopChild = std::make_shared<TreeNode>(REDO_LOOP); // New loop child

        auto child3 = std::make_shared<TreeNode>(ACTIVITY, "a");
        auto child4 = std::make_shared<TreeNode>(ACTIVITY, "b");
        auto child5 = std::make_shared<TreeNode>(ACTIVITY, "c");
        auto child6 = std::make_shared<TreeNode>(ACTIVITY, "d");
        auto child7 = std::make_shared<TreeNode>(ACTIVITY, "e");
        auto child8 = std::make_shared<TreeNode>(ACTIVITY, "f"); // New activity child
        auto child9 = std::make_shared<TreeNode>(ACTIVITY, "g"); // New activity child

        child1->addChild(child3);
        child1->addChild(child4);
        child1->addChild(child7);

        child2->addChild(child5);
        child2->addChild(child6);

        loopChild->addChild(child8); // Adding new activity child to loop child
        loopChild->addChild(child9); // Adding new activity child to loop child

        root->addChild(child1);
        root->addChild(child2);
        root->addChild(loopChild); // Adding loop child to root

        root->fillLetterMaps();

        SECTION("dbcda")
        {

            std::string trace = "dbcda";
            std::vector<std::vector<int>> expected = {{-1, -1, 4}, {-1, 0, 4}, {-1, 3, 4}, {1, 1, 4}, {1, 3, 4}, {4, 4, 4}};
            testPossibleSplits(root, trace, expected);
        }

        SECTION("dbcdaf")
        {

            std::string trace = "dbcdaf";
            std::vector<std::vector<int>> expected = {{-1, -1, 5}, {-1, 0, 5}, {-1, 3, 5}, {1, 1, 5}, {1, 3, 5}, {4, 4, 5}};
            testPossibleSplits(root, trace, expected);
        }
    }

    SECTION("Four Children")
    {
        auto root = std::make_shared<TreeNode>(SEQUENCE);

        auto child1 = std::make_shared<TreeNode>(PARALLEL);
        auto child2 = std::make_shared<TreeNode>(XOR);
        auto loopChild = std::make_shared<TreeNode>(REDO_LOOP); // New loop child
        auto xorChild = std::make_shared<TreeNode>(XOR);        // New xor child

        auto child3 = std::make_shared<TreeNode>(ACTIVITY, "a");
        auto child4 = std::make_shared<TreeNode>(ACTIVITY, "b");
        auto child5 = std::make_shared<TreeNode>(ACTIVITY, "c");
        auto child6 = std::make_shared<TreeNode>(ACTIVITY, "d");
        auto child7 = std::make_shared<TreeNode>(ACTIVITY, "e");
        auto child8 = std::make_shared<TreeNode>(ACTIVITY, "f");  // New activity child
        auto child9 = std::make_shared<TreeNode>(ACTIVITY, "g");  // New activity child
        auto child10 = std::make_shared<TreeNode>(ACTIVITY, "h"); // New activity child
        auto child11 = std::make_shared<TreeNode>(ACTIVITY, "i"); // New activity child

        child1->addChild(child3);
        child1->addChild(child4);
        child1->addChild(child7);

        child2->addChild(child5);
        child2->addChild(child6);

        loopChild->addChild(child8); // Adding new activity child to loop child
        loopChild->addChild(child9); // Adding new activity child to loop child

        xorChild->addChild(child10);
        xorChild->addChild(child11);

        root->addChild(child1);
        root->addChild(child2);
        root->addChild(loopChild); // Adding loop child to root
        root->addChild(xorChild);  // Adding xor child to root

        root->fillLetterMaps();

        SECTION("")
        {
            std::string trace = "";
            std::vector<std::vector<int>> expected = {{-1, -1, -1, -1}};
            testPossibleSplits(root, trace, expected);
        }

        SECTION("abecdfgi")
        {

            std::string trace = "abecdfghi";
            std::vector<std::vector<int>> expected =
                {{-1, -1, -1, 8}, {-1, -1, 6, 8}, {-1, 4, 4, 8}, {-1, 4, 6, 8}, {2, 2, 2, 8}, {2, 2, 6, 8}, {2, 4, 4, 8}, {2, 4, 6, 8}};
            testPossibleSplits(root, trace, expected);
        }

        SECTION("aacfhhf")
        {
            std::string trace = "aacfhhf";
            std::vector<std::vector<int>> expected = {{-1, -1, -1, 6}, {-1, -1, 3, 6}, {-1, -1, 6, 6}, {-1, 2, 2, 6}, {-1, 2, 3, 6}, {-1, 2, 6, 6}, {1, 1, 1, 6}, {1, 1, 3, 6}, {1, 1, 6, 6}, {1, 2, 2, 6}, {1, 2, 3, 6}, {1, 2, 6, 6}};
            testPossibleSplits(root, trace, expected);
        }

        SECTION("aagghi")
        {
            std::string trace = "aagghi";
            std::vector<std::vector<int>> expected = {{-1, -1, -1, 5}, {-1, -1, 3, 5}, {1, 1, 1, 5}, {1, 1, 3, 5}};
            testPossibleSplits(root, trace, expected);
        }

        SECTION("h")
        {
            std::string trace = "h";
            std::vector<std::vector<int>> expected = {{-1, -1, -1, 0}};
            testPossibleSplits(root, trace, expected);
        }
    }
}

TEST_CASE("spitTrace")
{
    SECTION("Empty trace")
    {
        std::string trace = "";
        std::vector<int> segment = {-1};
        std::vector<std::string> expected = {""};
        REQUIRE(segmentTrace(trace, segment) == expected);
    }

    SECTION("abcdcedffg")
    {
        std::string trace = "abcdcedffg";

        SECTION("normal split")
        {
            std::vector<int> segment = {0, 3, 6, 9};
            std::vector<std::string> expected = {"a", "bcd", "ced", "ffg"};
            REQUIRE(segmentTrace(trace, segment) == expected);
        }
        SECTION("mid empty split")
        {
            std::vector<int> segment = {0, 3, 3, 9};
            std::vector<std::string> expected = {"a", "bcd", "", "cedffg"};
            REQUIRE(segmentTrace(trace, segment) == expected);
        }
        SECTION("start empty split")
        {
            std::vector<int> segment = {-1, -1, 2, 9};
            std::vector<std::string> expected = {"", "", "abc", "dcedffg"};
            REQUIRE(segmentTrace(trace, segment) == expected);
        }
        SECTION("ebad")
        {
            trace = "ebad";
            std::vector<int> segment = {2,3};
            std::vector<std::string> expected = {"eba", "d"};
            REQUIRE(segmentTrace(trace, segment) == expected);
        }
    }
}

TEST_CASE("dynAlign")
{
    auto root = createExample();
    root->fillLetterMaps(); 

    SECTION("Empty trace")
    {
        const std::string trace = "";
        REQUIRE(dynAlign(root, trace) == 4);
    }   
    
    SECTION("eba")
    {
        const std::string trace = "eba";
        REQUIRE(dynAlign(root, trace) == 1);
    }   

    SECTION("ebad")
    {
        const std::string trace = "ebad";
        REQUIRE(dynAlign(root, trace) == 0);
    }   

    SECTION("babebbdddcbb")
    {
        const std::string trace = "babebbdddcbb";
        REQUIRE(dynAlign(root, trace) == 8);
    }
}