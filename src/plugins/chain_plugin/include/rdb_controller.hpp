#ifndef GRUUT_PUBLIC_MERGER_DB_CONTROLLER_HPP
#define GRUUT_PUBLIC_MERGER_DB_CONTROLLER_HPP

#include "../../../../lib/log/include/log.hpp"
#include "../config/storage_config.hpp"
#include "../config/storage_type.hpp"
#include "../structure/block.hpp"
#include "mysql/soci-mysql.h"
#include "soci.h"
#include "unresolved_block_pool.hpp"

#include <vector>

using namespace std;

namespace gruut {

class RdbController {
private:
  string m_dbms;
  string m_table_name;
  string m_db_user_id;
  string m_db_password;
  soci::connection_pool m_db_pool;

  Block rowToBlock(const soci::row &r);

public:
  RdbController(string_view dbms, string_view table_name, string_view db_user_id, string_view db_password);
  soci::connection_pool &pool();
  bool insertBlockData(Block &block);
  bool insertTransactionData(Block &block);
  bool insertUserLedgerData(std::map<string, user_ledger_type> &user_ledger);
  bool insertContractLedgerData(std::map<string, contract_ledger_type> &contract_ledger);

  vector<Block> getBlocks(const string &condition);
  Block getBlock(const string &condition);
  string getUserCert(const base58_type &user_id);

  bool queryUserJoin(UnresolvedBlock &UR_block, nlohmann::json &option, result_query_info_type &result_info);
  bool queryUserCert(UnresolvedBlock &UR_block, nlohmann::json &option, result_query_info_type &result_info);
  bool queryContractNew(UnresolvedBlock &UR_block, nlohmann::json &option, result_query_info_type &result_info);
  bool queryContractDisable(UnresolvedBlock &UR_block, nlohmann::json &option, result_query_info_type &result_info);

  bool queryIncinerate(std::map<string, user_ledger_type> &user_ledger, nlohmann::json &option, result_query_info_type &result_info);
  bool queryCreate(std::map<string, user_ledger_type> &user_ledger, nlohmann::json &option, result_query_info_type &result_info);
  bool queryTransfer(std::vector<LedgerRecord> &mem_ledger, nlohmann::json &option, result_query_info_type &result_info);
  bool queryUserScope(std::map<string, user_ledger_type> &user_ledger, nlohmann::json &option, result_query_info_type &result_info);
  bool queryContractScope(std::map<string, contract_ledger_type> &contract_ledger, nlohmann::json &option, result_query_info_type &result_info);
  bool queryTradeItem(std::vector<LedgerRecord> &mem_ledger, nlohmann::json &option, result_query_info_type &result_info);
  bool queryTradeVal(std::vector<LedgerRecord> &mem_ledger, nlohmann::json &option, result_query_info_type &result_info);
  bool queryRunQuery(std::vector<LedgerRecord> &mem_ledger, nlohmann::json &option, result_query_info_type &result_info);
  bool queryRunContract(std::vector<LedgerRecord> &mem_ledger, nlohmann::json &option, result_query_info_type &result_info);
};
} // namespace gruut
#endif
