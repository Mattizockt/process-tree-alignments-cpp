#define CATCH_CONFIG_MAIN
#include <catch2/catch_all.hpp>
#include "../src/utils.h"
#include "../src/treeNode.h"

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

            std::vector<std::vector<int>> splits = possibleSplits(root, trace);
            printNestedVector(splits);

            std::vector<std::vector<int>> expected = {{1, 2, 4}, {1, 3, 4}, {4, 4, 4}, {-1, -1, 4}, {-1, 2, 4}, {-1, 3, 4}, {-1, 0, 4}, {1, 1, 4}};

            std::sort(splits.begin(), splits.end());
            std::sort(expected.begin(), expected.end());

            REQUIRE(splits == expected);
        }

        SECTION("dbcdaf")
        {

            std::string trace = "dbcdaf";

            std::vector<std::vector<int>> splits = possibleSplits(root, trace);
            printNestedVector(splits);

            std::vector<std::vector<int>> expected = {{1, 2, 5}, {1, 3, 5}, {4, 4, 5}, {-1, -1, 5}, {-1, 2, 5}, {-1, 3, 5}, {-1, 0, 5}, {1, 1, 5}};

            std::sort(splits.begin(), splits.end());
            std::sort(expected.begin(), expected.end());

            REQUIRE(splits == expected);
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

            std::string trace = "abecdfgi";

            std::vector<std::vector<int>> splits = possibleSplits(root, trace);
            printNestedVector(splits);

            // TODO problem: even though 1,2,5 and 1,3,5 might belong to the same children they are still outputted. perhaps make it to 1,3,5 in the future. 
            std::vector<std::vector<int>> expected = {
                {0, 3, 5, 8}, {0, 3, 6, 8}, {0, 4, 5, 8}, {0, 4, 6, 8}, {1, 3, 5, 8}, {1, 3, 6, 8}, {1, 4, 5, 8}, {1, 4, 6, 8}, {2, 3, 5, 8}, {2, 3, 6, 8}, {2, 4, 5, 8}, {2, 4, 6, 8}, {-1, 3, 5, 8}, {-1, 3, 6, 8}, {-1, -1, 5, 8}, {-1, -1, 6, 8}, {-1, -1, -1, 8}};

            std::sort(splits.begin(), splits.end());
            std::sort(expected.begin(), expected.end());

            REQUIRE(splits == expected);
        }
    }
}