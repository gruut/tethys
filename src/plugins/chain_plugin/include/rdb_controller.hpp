#ifndef GRUUT_PUBLIC_MERGER_DB_CONTROLLER_HPP
#define GRUUT_PUBLIC_MERGER_DB_CONTROLLER_HPP

#include "../../../../lib/log/include/log.hpp"
#include "../config/storage_config.hpp"
#include "../config/storage_type.hpp"
#include "../structure/block.hpp"
#include "soci.h"

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
  bool insertBlockData(Block &block);
  bool insertTransactionData(Block &block);
  bool updateData(const string &userId, const string &varType, const string &varName, const string &varValue);
  bool deleteData(const string &userId, const string &varType, const string &varName);
  vector<Block> getBlocks(const string &condition);
  Block getBlock(const string &condition);
  string getUserCert(const base58_type &user_id);
};
} // namespace tethys
#endif
