#ifndef GRUUT_PUBLIC_MERGER_DB_LIB_HPP
#define GRUUT_PUBLIC_MERGER_DB_LIB_HPP

#include "../../../../lib/json/include/json.hpp"
#include "mysql/soci-mysql.h"
#include "soci.h"

#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <utility>

using namespace std;

namespace gruut {

class mariaDb {
private:
  soci::session m_db_session;
  soci::row row;

public:
  mariaDb() : m_db_session("mysql", "db=gruut_development user=gruut_user password='1234'") {
    cout << "*** mariaDb() is called" << endl;
  }

  ~mariaDb() {
    cout << "*** ~mariaDb() is called" << endl;
  }

  int performQuery(string &query);
  int disconnect();

  int insertData(string blockId, string userId, string varType, string varName, string varValue, string path);
  int updateData(string userId, string varType, string varName, string varValue);
  int deleteData(string userId, string varType, string varName);

  int checkUserIdVarTypeVarName(string *userId, string *varType, string *varName);
};

} // namespace gruut

#endif // WORKSPACE_DB_LIB_HPP
