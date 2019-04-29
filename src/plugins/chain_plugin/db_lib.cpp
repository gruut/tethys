#include "include/db_lib.hpp"

namespace gruut {

int mariaDb::performQuery(string &query) {
  try {
    m_db_session << query;
    cout << query << endl;
  } catch (soci::mysql_soci_error const &e) {
    cout << "error in performQuery() function: " << endl;
    cerr << "MySQL error: " << e.err_num_ << " " << e.what() << endl;
  } catch (exception const &e) {
    cerr << "Some other error: " << e.what() << endl;
  }

  return 0;
}

int mariaDb::disconnect() {
  m_db_session.close();
  cout << "*** Disconnected." << endl;
  return 0;
}

int mariaDb::insertData(string blockId, string userId, string varType, string varName, string varValue, string path) {
  if (checkUserIdVarTypeVarName(&userId, &varType, &varName) == 1) {
    string query = "INSERT INTO ledger (block_id, user_id, var_type, var_name, var_value, merkle_path) VALUES('" + blockId + "', '" +
                   userId + "', '" + varType + "', '" + varName + "', '" + varValue + "', '" + path + "')";
    if (performQuery(query) == 0) {
      cout << "insert() function was processed!!!" << endl;
      return 0;
    } else {
      cout << "insert() function was not processed!!!" << endl;
      return 1;
    }
  } else {
    cout << "insert() function was not processed!!!" << endl;
    return 1;
  }
}

int mariaDb::updateData(string userId, string varType, string varName, string varValue) {
  if (checkUserIdVarTypeVarName(&userId, &varType, &varName) == 0) {
    string query = "UPDATE ledger SET var_value='" + varValue + "' WHERE user_id='" + userId + "' AND var_type='" + varType +
                   "' AND var_name='" + varName + "'";
    if (performQuery(query) == 0) {
      cout << "updateVarValue() function was processed!!!" << endl;
      return 0;
    } else {
      cout << "updateVarValue() function was not processed!!!" << endl;
      return 1;
    }
  } else {
    cout << "updateVarValue() function was not processed!!!" << endl;
    return 1;
  }
}

int mariaDb::deleteData(string userId, string varType, string varName) {
  if (checkUserIdVarTypeVarName(&userId, &varType, &varName) == 0) {
    string query = "DELETE FROM ledger WHERE user_id='" + userId + "' AND var_type='" + varType + "' AND var_name='" + varName + "'";
    if (performQuery(query) == 0) {
      cout << "deleteData() function was processed!!!" << endl;
      return 0;
    } else {
      cout << "deleteData() function was not processed!!!" << endl;
      return 1;
    }
  } else {
    cout << "deleteData() function was not processed!!!" << endl;
    return 1;
  }
}

int mariaDb::checkUserIdVarTypeVarName(string *userId, string *varType, string *varName) {
  string query = "SELECT user_id, var_type, var_name FROM ledger WHERE user_id='" + *userId + "' AND var_type='" + *varType +
                 "' AND var_name='" + *varName + "'";
  if (performQuery(query) == 0) {
    columns = mysql_num_fields(res); // the number of field
    if ((row = mysql_fetch_row(res)) != NULL) {
      cout << row[0] << ", " << row[1] << ", " << row[2] << endl;
      cout << "exists." << endl;
      return 0;
    } else {
      cout << *userId << ", " << *varType << ", " << *varName << endl;
      cout << "does not exist." << endl;
      return 1;
    }
  }
  return 0;
}

}