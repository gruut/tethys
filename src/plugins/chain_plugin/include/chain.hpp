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
  string getUserCert(const base58_type &user_id);
  void insertBlockData(Block &block_info);
  void insertTransactionData(Block &block_info);
  void insertUserLedgerData(std::map<string, user_ledger_type> &user_ledger);
  void insertContractLedgerData(std::map<string, contract_ledger_type> &contract_ledger);

  bool queryUserJoin(UnresolvedBlock &UR_block, nlohmann::json &option, result_query_info_type &result_info);
  bool queryUserCert(UnresolvedBlock &UR_block, nlohmann::json &option, result_query_info_type &result_info);
  bool queryContractNew(UnresolvedBlock &UR_block, nlohmann::json &option, result_query_info_type &result_info);
  bool queryContractDisable(UnresolvedBlock &UR_block, nlohmann::json &option, result_query_info_type &result_info);

  // KV functions
  void saveWorld(world_type &world_info);
  void saveChain(local_chain_type &chain_info);
  void saveBackup(UnresolvedBlock &block_info);
  void saveSelfInfo(self_info_type &self_info);
  vector<Block> getBlocksByHeight(int from, int to);
  block_height_type getLatestResolvedHeight();
  string getValueByKey(DataType what, const string &base_keys);

  // Unresolved block pool functions
  bool queryIncinerate(UnresolvedBlock &UR_block, nlohmann::json &option, result_query_info_type &result_info);
  bool queryCreate(UnresolvedBlock &UR_block, nlohmann::json &option, result_query_info_type &result_info);
  bool queryTransfer(UnresolvedBlock &UR_block, nlohmann::json &option, result_query_info_type &result_info);
  bool queryUserScope(UnresolvedBlock &UR_block, nlohmann::json &option, result_query_info_type &result_info);
  bool queryContractScope(UnresolvedBlock &UR_block, nlohmann::json &option, result_query_info_type &result_info);
  bool queryTradeItem(UnresolvedBlock &UR_block, nlohmann::json &option, result_query_info_type &result_info);
  bool queryTradeVal(UnresolvedBlock &UR_block, nlohmann::json &option, result_query_info_type &result_info);
  bool queryRunQuery(UnresolvedBlock &UR_block, nlohmann::json &option, result_query_info_type &result_info);
  bool queryRunContract(UnresolvedBlock &UR_block, nlohmann::json &option, result_query_info_type &result_info);
  string pidCheck(optional<string> pid, string var_name, int var_type, string var_owner);
  user_ledger_type findUserLedgerFromPoint(string key, int deq_idx, int vec_idx);

  ubp_push_result_type pushBlock(Block &block, bool is_restore = false);
  UnresolvedBlock findBlock(const base58_type &block_id, const block_height_type block_height);
  bool resolveBlock(Block &block, UnresolvedBlock &resolved_result);
  base58_type getCurrentHeadId();
  bytes getUserStateRoot();
  bytes getContractStateRoot();
  void setPool(const base64_type &last_block_id, block_height_type last_height, timestamp_t last_time, const base64_type &last_hash,
               const base64_type &prev_block_id);

private:
  unique_ptr<class ChainImpl> impl;
  unique_ptr<RdbController> rdb_controller;
  unique_ptr<KvController> kv_controller;
  unique_ptr<UnresolvedBlockPool> unresolved_block_pool;
};
} // namespace gruut
