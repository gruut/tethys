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
#include <optional>
#include <utility>
#include <vector>

namespace tethys {

using namespace std;

class Chain {
private:
  unique_ptr<class ChainImpl> impl;
  unique_ptr<RdbController> rdb_controller;
  unique_ptr<KvController> kv_controller;
  unique_ptr<UnresolvedBlockPool> unresolved_block_pool;

public:
  Chain(string_view dbms, string_view table_name, string_view db_user_id, string_view db_password);
  ~Chain();

  Chain(const Chain &) = delete;
  Chain(const Chain &&) = delete;
  Chain &operator=(const Chain &) = delete;

  void initWorld(nlohmann::json &world_state);
  optional<vector<string>> initChain(nlohmann::json &chain_state);

  // RDB functions
  string getUserCert(const base58_type &user_id);
  bool applyBlockToRDB(const Block &block_info);
  bool applyTransactionToRDB(const Block &block_info);
  bool applyUserLedgerToRDB(const map<string, user_ledger_type> &user_ledger_list);
  bool applyContractLedgerToRDB(const map<string, contract_ledger_type> &contract_ledger_list);
  bool applyUserAttributeToRDB(const map<base58_type, user_attribute_type> &user_attribute_list);
  bool applyUserCertToRDB(const map<base58_type, user_cert_type> &user_cert_list);
  bool applyContractToRDB(const map<base58_type, contract_type> &contract_list);

  bool findUserFromRDB(string key, user_ledger_type &user_ledger);
  bool findContractFromRDB(string key, contract_ledger_type &contract_ledger);
  int getVarType(string &key);

  // KV functions
  void saveLatestWorldId(const alphanumeric_type &world_id);
  void saveLatestChainId(const alphanumeric_type &chain_id);
  void saveWorld(world_type &world_info);
  void saveChain(local_chain_type &chain_info);
  void saveBlockIds();
  void saveBackupBlock(const nlohmann::json &block_json);
  void saveBackupResult(const UnresolvedBlock &UR_block);
  void saveSelfInfo(self_info_type &self_info);
  vector<Block> getBlocksByHeight(int from, int to);
  block_height_type getLatestResolvedHeight();
  string getValueByKey(string what, const string &base_keys);
  void restorePool();
  void restoreUserLedgerList(UnresolvedBlock &restored_unresolved_block, const nlohmann::json &user_ledger_list_json);
  void restoreContractLedgerList(UnresolvedBlock &restored_unresolved_block, const nlohmann::json &contract_ledger_list_json);
  void restoreUserAttributeList(UnresolvedBlock &restored_unresolved_block, const nlohmann::json &user_attribute_list_json);
  void restoreUserCertList(UnresolvedBlock &restored_unresolved_block, const nlohmann::json &user_cert_list_json);
  void restoreContractList(UnresolvedBlock &restored_unresolved_block, const nlohmann::json &contract_list_json);

  // Unresolved block pool functions
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
  string pidCheck(optional<string> pid, string var_name, int var_type, string var_owner);
  search_result_type findUserLedgerFromPoint(string key, block_height_type height, int vec_idx);
  search_result_type findContractLedgerFromPoint(string key, block_height_type height, int vec_idx);

  block_push_result_type pushBlock(Block &block);
  UnresolvedBlock findBlock(const base58_type &block_id, const block_height_type block_height);
  bool resolveBlock(Block &block, UnresolvedBlock &resolved_result);
  void setPool(const base64_type &last_block_id, block_height_type last_height, timestamp_t last_time, const base64_type &last_hash,
               const base64_type &prev_block_id);

  // State tree
private:
  StateTree m_us_tree; // user scope state tree
  StateTree m_cs_tree; // contract scope state tree
  base64_type m_head_id;
  block_height_type m_head_height;
  int m_head_deq_idx;
  int m_head_vec_idx;

public:
  void setupStateTree();
  void updateStateTree(const UnresolvedBlock &unresolved_block);
  void revertStateTree(const UnresolvedBlock &unresolved_block);

  user_ledger_type findUserLedgerFromHead(string key);
  contract_ledger_type findContractLedgerFromHead(string key);
  void moveHead(const base58_type &target_block_id, const block_height_type target_block_height);
  base58_type getCurrentHeadId();
  block_height_type getCurrentHeadHeight();
  bytes getUserStateRoot();
  bytes getContractStateRoot();
};
} // namespace tethys
