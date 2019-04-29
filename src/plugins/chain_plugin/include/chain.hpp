#pragma once

#include "../../../../include/json.hpp"
#include "mysql/soci-mysql.h"
#include "soci.h"
#include <boost/program_options/variables_map.hpp>
#include <memory>

namespace gruut {

using namespace std;

class Chain {
public:
  Chain(const string &dbms, const string &table_name, const string &db_user_id, const string &db_password);
  ~Chain();

  Chain(const Chain &) = delete;
  Chain(const Chain &&) = delete;
  Chain &operator=(const Chain &) = delete;

  void startup(nlohmann::json &genesis_state);

private:
  soci::session db_session;
  unique_ptr<class ChainImpl> impl;
};
} // namespace gruut
