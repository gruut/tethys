#pragma once

#include "../../../../include/json.hpp"
#include "../../../../lib/log/include/log.hpp"
#include "../config/storage_type.hpp"
#include "../structure/block.hpp"
#include "kv_store.hpp"
#include "rdb_controller.hpp"
#include "unresolved_block_pool.hpp"

#include <boost/program_options/variables_map.hpp>
#include <memory>
#include <utility>
#include <vector>

namespace gruut {

using namespace std;

class Chain {
public:
  Chain(string_view dbms, string_view table_name, string_view db_user_id, string_view db_password);
  ~Chain();

  Chain(const Chain &) = delete;
  Chain(const Chain &&) = delete;
  Chain &operator=(const Chain &) = delete;

  void startup(nlohmann::json &genesis_state);

  // RDB functions
  void insertBlockData(Block &first_block);

  // KV functions
  void saveWorld(world_type &world_info);
  void saveChain(local_chain_type &chain_info);
  void saveBackup(UnresolvedBlock &block_info);
  vector<Block> getBlocksByHeight(int from, int to);
  block_height_type getLatestResolvedHeight();
  string getValueByKey(DataType what, const string &base_keys);

private:
  unique_ptr<class ChainImpl> impl;
  unique_ptr<RdbController> rdb_controller;
  unique_ptr<KvController> kv_controller;
};
} // namespace gruut
