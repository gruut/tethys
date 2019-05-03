#ifndef GRUUT_PUBLIC_MERGER_DB_CONTROLLER_HPP
#define GRUUT_PUBLIC_MERGER_DB_CONTROLLER_HPP

#include "../config/config.hpp"
#include "../config/storage_type.hpp"
#include "mysql/soci-mysql.h"
#include "soci.h"
#include "../../../../lib/log/include/log.hpp"

using namespace std;

namespace gruut {

class DBController {
private:
  string m_dbms;
  string m_table_name;
  string m_db_user_id;
  string m_db_password;
  soci::connection_pool m_db_pool;

public:
  DBController(string_view dbms, string_view table_name, string_view db_user_id, string_view db_password);
  int performQuery(const string &query);
  bool insertData(const string &blockId, const string &userId, const string &varType, const string &varName, const string &varValue,
                  const string &path);
  bool updateData(const string &userId, const string &varType, const string &varName, const string &varValue);
  bool deleteData(const string &userId, const string &varType, const string &varName);
};
} // namespace gruut
#endif
