#define CATCH_CONFIG_MAIN

#include "../../../include/catch.hpp"
#include "../include/json.hpp"
#include <iostream>

TEST_CASE("json::parse") {
  SECTION("it should parse JSON string") {
    auto json_obj = json::parse("{ \"happy\": true, \"pi\": 3.141 }");

    REQUIRE(json_obj["happy"] == true);
    REQUIRE(json_obj["pi"] == 3.141);
  }
}

TEST_CASE("json::is_empty") {
  SECTION("it should not throw a exception if the input is not valid JSON format") {
    auto json_obj = json::parse("}");

    REQUIRE(json::is_empty(json_obj) == true);
  }
}
