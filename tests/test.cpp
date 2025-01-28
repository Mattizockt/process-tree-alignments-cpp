#define CATCH_CONFIG_MAIN
#include <catch2/catch_all.hpp>
#include "../src/utils.h"
#include "../src/treeNode.h"

void testPossibleSplits(const std::shared_ptr<TreeNode> &root, const std::string &trace, const std::vector<std::vector<int>> &expected)
{
    auto splits = possibleSplits(root, trace);
    printNestedVector(splits);
    std::sort(splits.begin(), splits.end());
    auto sortedExpected = expected;
    std::sort(sortedExpected.begin(), sortedExpected.end());
    REQUIRE(splits == sortedExpected);
}

TEST_CASE("Does possibleSplits work?")
{

    SECTION("Single Child")
    {
    }

    SECTION("Two Children")
    {
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
            std::vector<std::vector<int>> expected = {{1, 2, 4}, {1, 3, 4}, {4, 4, 4}, {-1, -1, 4}, {-1, 2, 4}, {-1, 3, 4}, {-1, 0, 4}, {1, 1, 4}};
            testPossibleSplits(root, trace, expected);
        }

        SECTION("dbcdaf")
        {

            std::string trace = "dbcdaf";
            std::vector<std::vector<int>> expected = {{1, 2, 5}, {1, 3, 5}, {4, 4, 5}, {-1, -1, 5}, {-1, 2, 5}, {-1, 3, 5}, {-1, 0, 5}, {1, 1, 5}};
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

        SECTION("abecdfgi")
        {

            std::string trace = "abecdfghi";
            // TODO problem: even though 1,2,5 and 1,3,5 might belong to the same children they are still outputted. perhaps make it to 1,3,5 in the future.
            std::vector<std::vector<int>> expected =
                {{-1, -1, -1, 8}, {-1, -1, 5, 8}, {-1, -1, 6, 8}, {-1, 3, 3, 8}, {-1, 3, 5, 8}, {-1, 3, 6, 8}, {-1, 4, 4, 8}, {-1, 4, 5, 8}, {-1, 4, 6, 8}, {0, 0, 0, 8}, {0, 0, 5, 8}, {0, 0, 6, 8}, {0, 3, 3, 8}, {0, 3, 5, 8}, {0, 3, 6, 8}, {0, 4, 4, 8}, {0, 4, 5, 8}, {0, 4, 6, 8}, {1, 1, 1, 8}, {1, 1, 5, 8}, {1, 1, 6, 8}, {1, 3, 3, 8}, {1, 3, 5, 8}, {1, 3, 6, 8}, {1, 4, 4, 8}, {1, 4, 5, 8}, {1, 4, 6, 8}, {2, 2, 2, 8}, {2, 2, 5, 8}, {2, 2, 6, 8}, {2, 3, 3, 8}, {2, 3, 5, 8}, {2, 3, 6, 8}, {2, 4, 4, 8}, {2, 4, 5, 8}, {2, 4, 6, 8}};
            testPossibleSplits(root, trace, expected);
        }

        SECTION("aacfhhf")
        {
            std::string trace = "aacfhhf";
            std::vector<std::vector<int>> expected = {{-1, -1, -1, 6}, {-1, -1, 3, 6}, {-1, -1, 6, 6}, {-1, 2, 2, 6}, {-1, 2, 3, 6}, {-1, 2, 6, 6}, {0, 0, 0, 6}, {0, 0, 3, 6}, {0, 0, 6, 6}, {0, 2, 2, 6}, {0, 2, 3, 6}, {0, 2, 6, 6}, {1, 1, 1, 6}, {1, 1, 3, 6}, {1, 1, 6, 6}, {1, 2, 2, 6}, {1, 2, 3, 6}, {1, 2, 6, 6}};
            testPossibleSplits(root, trace, expected);
        }

        SECTION("aagghi")
        {
            std::string trace = "aagghi";
            std::vector<std::vector<int>> expected = {{-1, -1, -1, 5}, {-1, -1, 2, 5}, {-1, -1, 3, 5}, {0, 0, 0, 5}, {0, 0, 2, 5}, {0, 0, 3, 5}, {1, 1, 1, 5}, {1, 1, 2, 5}, {1, 1, 3, 5}};
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