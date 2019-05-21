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
  void insertBlockData(Block &block_info);
  void insertTransactionData(Block &block_info);
  string getUserCert(const base58_type &user_id);

  bool queryUserJoin(UnresolvedBlock &UR_block, nlohmann::json &option, result_query_info_type &result_info);
  bool queryUserCert(UnresolvedBlock &UR_block, nlohmann::json &option, result_query_info_type &result_info);
  bool queryContractNew(UnresolvedBlock &UR_block, nlohmann::json &option, result_query_info_type &result_info);
  bool queryContractDisable(UnresolvedBlock &UR_block, nlohmann::json &option, result_query_info_type &result_info);
  bool queryIncinerate(UnresolvedBlock &UR_block, nlohmann::json &option, result_query_info_type &result_info);
  bool queryCreate(UnresolvedBlock &UR_block, nlohmann::json &option, result_query_info_type &result_info);
  bool queryTransfer(UnresolvedBlock &UR_block, nlohmann::json &option, result_query_info_type &result_info);
  bool queryUserScope(UnresolvedBlock &UR_block, nlohmann::json &option, result_query_info_type &result_info);
  bool queryContractScope(UnresolvedBlock &UR_block, nlohmann::json &option, result_query_info_type &result_info);
  bool queryTradeItem(UnresolvedBlock &UR_block, nlohmann::json &option, result_query_info_type &result_info);
  bool queryTradeVal(UnresolvedBlock &UR_block, nlohmann::json &option, result_query_info_type &result_info);
  bool queryRunQuery(UnresolvedBlock &UR_block, nlohmann::json &option, result_query_info_type &result_info);
  bool queryRunContract(UnresolvedBlock &UR_block, nlohmann::json &option, result_query_info_type &result_info);

  // KV functions
  void saveWorld(world_type &world_info);
  void saveChain(local_chain_type &chain_info);
  void saveBackup(UnresolvedBlock &block_info);
  void saveSelfInfo(self_info_type &self_info);
  vector<Block> getBlocksByHeight(int from, int to);
  block_height_type getLatestResolvedHeight();
  string getValueByKey(DataType what, const string &base_keys);

private:
  unique_ptr<class ChainImpl> impl;
  unique_ptr<RdbController> rdb_controller;
  unique_ptr<KvController> kv_controller;
};
} // namespace gruut
