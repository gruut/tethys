#pragma once

#include "../../../include/json.hpp"

using namespace std;

namespace json {
static auto parse(const string &str) {
  return nlohmann::json::parse(str, nullptr, false);
}

static auto is_empty(const nlohmann::json &j) {
  return j.is_discarded();
}
} // namespace json
