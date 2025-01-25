#define CATCH_CONFIG_MAIN
#include <catch2/catch_all.hpp>

// Functions to test
int add(int a, int b) { return a + b; }
bool is_even(int n) { return n % 2 == 0; }

// Tests
TEST_CASE("Addition works", "[math]") {
    REQUIRE(add(2, 2) == 4);
    REQUIRE(add(-1, 1) == 0);
}

TEST_CASE("Even number detection", "[utility]") {
    REQUIRE(is_even(2));
    REQUIRE(!is_even(3));
}