#ifndef TETHYS_PUBLIC_MERGER_DB_CONTROLLER_HPP
#define TETHYS_PUBLIC_MERGER_DB_CONTROLLER_HPP

#include "../../../../lib/log/include/log.hpp"
#include "../config/storage_config.hpp"
#include "../config/storage_type.hpp"
#include "../structure/block.hpp"
#include "soci.h"
#include "unresolved_block_pool.hpp"

#include <vector>

using namespace std;

namespace tethys {

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
  bool applyBlockToRDB(const Block &block);
  bool applyTransactionToRDB(const Block &block);
  bool applyUserLedgerToRDB(const map<string, user_ledger_type> &user_ledger_list);
  bool applyContractLedgerToRDB(const map<string, contract_ledger_type> &contract_ledger_list);
  bool applyUserAttributeToRDB(const map<base58_type, user_attribute_type> &user_attribute_list);
  bool applyUserCertToRDB(const map<base58_type, user_cert_type> &user_cert_list);
  bool applyContractToRDB(const map<base58_type, contract_type> &contract_list);

  vector<Block> getBlocks(const string &condition);
  Block getBlock(const string &condition);
  string getUserCert(const base58_type &user_id);

  //  bool queryRunQuery(std::vector<LedgerRecord> &mem_ledger, nlohmann::json &option, result_query_info_type &result_info);
  //  bool queryRunContract(std::vector<LedgerRecord> &mem_ledger, nlohmann::json &option, result_query_info_type &result_info);

  bool checkUniqueVarNameFromRDB(const string &var_owner, const string &var_name);
  bool findUserFromRDB(const string &pid, user_ledger_type &user_ledger);
  bool findContractFromRDB(const string &pid, contract_ledger_type &contract_ledger);
  int getVarTypeFromRDB(const string &pid);

  vector<user_ledger_type> getAllUserLedger();
  vector<contract_ledger_type> getAllContractLedger();
};
} // namespace tethys
#endif
