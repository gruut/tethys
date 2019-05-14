#pragma once

#include "../../../../include/json.hpp"
#include "../../../../lib/log/include/log.hpp"
#include "../config/storage_type.hpp"
#include "kv_store.hpp"
#include "rdb_controller.hpp"

#include <boost/program_options/variables_map.hpp>
#include <memory>
#include <utility>

namespace gruut {

using namespace std;

class Chain {
public:
  unique_ptr<RdbController> rdb_controller;
  unique_ptr<KvController> kv_controller;

  Chain(string_view dbms, string_view table_name, string_view db_user_id, string_view db_password);
  ~Chain();

  Chain(const Chain &) = delete;
  Chain(const Chain &&) = delete;
  Chain &operator=(const Chain &) = delete;

  void startup(nlohmann::json &genesis_state);

private:
  unique_ptr<class ChainImpl> impl;
};
} // namespace gruut
