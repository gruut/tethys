#pragma once

#include "mysql/soci-mysql.h"
#include "soci.h"

namespace gruut {

using namespace std;

class Chain {
public:
  Chain(string dbms, string table_name, string db_user_id, string db_password);

  Chain(const Chain &) = delete;
  Chain(const Chain &&) = delete;
  Chain &operator=(const Chain &) = delete;

private:
  soci::session db_session;
};
} // namespace gruut
