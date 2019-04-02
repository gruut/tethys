#pragma once

#include "../../../include/json.hpp"
#include <optional>
#include <type_traits>

using namespace std;

namespace json {
static auto parse(const string &str) {
  return nlohmann::json::parse(str, nullptr, false);
}

static auto is_empty(const nlohmann::json &j) {
  return j.is_discarded();
}

template <typename T>
static optional<T> get(const nlohmann::json &json_obj, const string &key) {
  if (json_obj.contains(key)) {
    if (is_convertible_v<decltype(json_obj[key]), T>) {
      return static_cast<T>(json_obj[key]);
    } else {
      return {};
    }
  } else {
    return {};
  }
}
} // namespace json
