#pragma once

#include "../../../include/json.hpp"
#include <optional>

using namespace std;

namespace json {
static auto parse(const string &str) {
  return nlohmann::json::parse(str, nullptr, false);
}

static auto is_empty(const nlohmann::json &j) {
  return j.is_discarded();
}

static optional<string> getString(const nlohmann::json &json_obj) {
  std::string ret_str;
  if (!json_obj.empty()) {
    try {
      return json_obj.get<std::string>();
    } catch (...) {
      /* do nothing */
    }
  }
  return {};
}

static optional<string> getString(const nlohmann::json &json_obj, const std::string &key) {
  if (json_obj.find(key) != json_obj.end()) {
    return getString(json_obj[key]);
  }
  return {};
}

static optional<int> getInt(nlohmann::json &json_obj, const string &key) {
  auto dec_str = getString(json_obj, key);
  if (dec_str.has_value())
    return stoi(dec_str.value());
  return {};
}

static optional<uint64_t> getUint64(nlohmann::json &json_obj, const string &key) {
  auto dec_str = getString(json_obj, key);
  if (dec_str.has_value())
    return static_cast<uint64_t>(stoll(dec_str.value()));
  return {};
}

static optional<bool> getBoolean(const nlohmann::json &json_obj, const std::string &key) {
  if (json_obj.find(key) != json_obj.end()) {
    if (!json_obj[key].is_boolean()) {
      return {};
    }
    return json_obj[key].get<bool>();
  }
  return {};
}
} // namespace json
