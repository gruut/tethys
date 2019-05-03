#include "include/db_controller.hpp"

using namespace std;

namespace gruut {

DBController::DBController(string_view dbms, string_view table_name, string_view db_user_id, string_view db_password)
    : m_dbms(dbms), m_table_name(table_name), m_db_user_id(db_user_id), m_db_password(db_password),
      m_db_pool(config::DB_SESSION_POOL_SIZE) {
  for (size_t i = 0; i != config::DB_SESSION_POOL_SIZE; ++i) {
    soci::session &sql = m_db_pool.at(i);
    sql.open(m_dbms, "service=" + m_table_name + " user=" + m_db_user_id + " password=" + m_db_password);
  }
}

int DBController::performQuery(const string &query) {
  try {
    soci::session db_session(m_db_pool);
    soci::row result;

    soci::statement st = (db_session.prepare << query, soci::into(result));
    st.execute(true);
    logger::INFO(query);
  } catch (soci::mysql_soci_error const &e) {
    logger::ERROR("error in performQuery() function: {}", e.what());
    logger::ERROR("MySQL error: {}", e.what());
  } catch (exception const &e) {
    logger::ERROR("Some other error: {}", e.what());
  }

  return 0;
}
//
//bool DBController::insertData(const string &blockId, const string &userId, const string &varType, const string &varName,
//                              const string &varValue, const string &path) {
//  if (checkUserIdVarTypeVarName(&userId, &varType, &varName) == 1) {
//    string query1 = "INSERT INTO ledger (block_id, user_id, var_type, var_name, var_value, merkle_path) VALUES('" + blockId + "', '" +
//                    userId + "', '" + varType + "', '" + varName + "', '" + varValue + "', '" + path + "')";
//    if (performQuery(query1) == 0) {
//      logger::INFO("insert() function was processed!!!");
//      return 0;
//    } else {
//      logger::INFO("insert() function was not processed!!!");
//      return 1;
//    }
//  } else {
//    logger::INFO("insert() function was not processed!!!");
//    return 1;
//  }
//}
//
//bool DBController::updateData(const string &userId, const string &varType, const string &varName, const string &varValue) {
//  if (checkUserIdVarTypeVarName(&userId, &varType, &varName) == 0) {
//    string query = "UPDATE ledger SET var_value='" + varValue + "' WHERE user_id='" + userId + "' AND var_type='" + varType +
//                   "' AND var_name='" + varName + "'";
//    if (performQuery(query) == 0) {
//      logger::INFO("updateVarValue() function was processed!!!");
//      return 0;
//    } else {
//      logger::INFO("updateVarValue() function was not processed!!!");
//      return 1;
//    }
//  } else {
//    logger::INFO("updateVarValue() function was not processed!!!");
//    return 1;
//  }
//}
//
//bool DBController::deleteData(const string &userId, const string &varType, const string &varName) {
//  if (checkUserIdVarTypeVarName(&userId, &varType, &varName) == 0) {
//    string query = "DELETE FROM ledger WHERE user_id='" + userId + "' AND var_type='" + varType + "' AND var_name='" + varName + "'";
//    if (performQuery(query) == 0) {
//      logger::INFO("deleteData() function was processed!!!");
//      return 0;
//    } else {
//      logger::INFO("deleteData() function was not processed!!!");
//      return 1;
//    }
//  } else {
//    logger::INFO("deleteData() function was not processed!!!");
//    return 1;
//  }
//}
}