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

TEST_CASE("json::get") {
  auto sample_json = R"(
        {
            "content":"tethys",
            "check" : true,
            "int_num" : -123456,
            "time_stamp" : 1543323592
        }
        )"_json;

  SECTION("it should get string from json obj safely") {
    REQUIRE(json::get<std::string>(sample_json, "content").has_value());
    REQUIRE(json::get<std::string>(sample_json, "content").value() == "tethys");
    REQUIRE(!json::get<std::string>(sample_json, "no-content").has_value());
  }
  SECTION("it should get boolean from json obj safely") {
    REQUIRE(json::get<bool>(sample_json, "check").has_value());
    REQUIRE(json::get<bool>(sample_json, "check").value() == true);
    REQUIRE(!json::get<bool>(sample_json, "no-check").has_value());
  }
  SECTION("it should get int from json obj safely") {
    REQUIRE(json::get<int>(sample_json, "int_num").has_value());
    REQUIRE(json::get<int>(sample_json, "int_num").value() == -123456);
    REQUIRE(!json::get<int>(sample_json, "no-int_num").has_value());
  }
  SECTION("it should get uint64_t from json obj safely") {
    REQUIRE(json::get<uint64_t>(sample_json, "time_stamp").has_value());
    REQUIRE(json::get<uint64_t>(sample_json, "time_stamp").value() == 1543323592);
    REQUIRE(!json::get<uint64_t>(sample_json, "no-time_stamp").has_value());

    REQUIRE(sizeof(json::get<uint64_t>(sample_json, "time_stamp").value()) == 8);
  }
}
