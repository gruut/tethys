#ifndef GRUUT_PUBLIC_MERGER_DB_CONTROLLER_HPP
#define GRUUT_PUBLIC_MERGER_DB_CONTROLLER_HPP

#include "../config/config.hpp"
#include "../config/storage_type.hpp"
#include "mysql/soci-mysql.h"
#include "soci.h"

namespace gruut {

class DBController {
public:
  soci::connection_pool m_db_pool;

private:
  string m_dbms;
  string m_table_name;
  string m_db_user_id;
  string m_db_password;

public:
  DBController(const string &dbms, const string &table_name, const string &db_user_id, const string &db_password)
      : m_dbms(dbms), m_table_name(table_name), m_db_user_id(db_user_id), m_db_password(db_password),
        m_db_pool(config::DB_SESSION_POOL_SIZE) {
    for (size_t i = 0; i != config::DB_SESSION_POOL_SIZE; ++i) {
      soci::session &sql = m_db_pool.at(i);
      sql.open(m_dbms, "service=" + m_table_name + " user=" + m_db_user_id + " password=" + m_db_password);
    }
  };
};
} // namespace gruut
#endif
