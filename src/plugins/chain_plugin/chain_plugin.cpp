#include "include/chain_plugin.hpp"
#include "../../../include/json.hpp"
#include "../../../lib/log/include/log.hpp"

#include "mysql/soci-mysql.h"
#include "soci.h"

namespace gruut {

namespace fs = boost::filesystem;

using namespace std;

class ChainPluginImpl {
public:
  string dbms;
  string table_name;
  string db_user_id;
  string db_password;

  nlohmann::json genesis_state;

  void initialize() {
    const string conn_opt = "service=" + table_name + " user=" + db_user_id + " password=" + db_password;

    soci::session sql(dbms, conn_opt);
  }


};

ChainPlugin::ChainPlugin() : impl(new ChainPluginImpl()) {}

void ChainPlugin::pluginInitialize(const boost::program_options::variables_map &options) {
  logger::INFO("ChainPlugin Initialize");

  if (options.count("genesis-block") > 0) {
    fs::path config_path = options.at("genesis-block").as<std::string>();

    if (config_path.is_relative())
      config_path = fs::current_path() / config_path;

    if (!fs::exists(config_path)) {
      logger::ERROR("Can't find a genesis block state file. (path: {})", config_path.string());
      throw std::invalid_argument("Cannot find a genesis_block.json"s);
    }

    std::ifstream i(config_path.make_preferred().string());

    i >> impl->genesis_state;
  } else {
    throw std::invalid_argument("genesis block state file does not exist"s);
  }

  if (options.count("dbms")) {
    impl->dbms = options.at("dbms").as<string>();
  } else {
    throw std::invalid_argument("the input of dbms is empty"s);
  }

  if (options.count("table-name")) {
    impl->table_name = options.at("table-name").as<string>();
  } else {
    throw std::invalid_argument("the input of table_name is empty"s);
  }

  if (options.count("database-user")) {
    impl->db_user_id = options.at("database-user").as<string>();
  } else {
    throw std::invalid_argument("the input of database's user_id is empty"s);
  }

  if (options.count("database-password")) {
    impl->db_password = options.at("database-password").as<string>();
  } else {
    throw std::invalid_argument("the input of database's password is empty"s);
  }

  impl->initialize();
}
void ChainPlugin::setProgramOptions(options_description &cfg) {
  cfg.add_options()("genesis-block", boost::program_options::value<string>()->composing(), "the location of a genesis_block.json file")
  ("dbms", boost::program_options::value<string>()->composing(), "DBMS (MYSQL)")
  ("table-name", boost::program_options::value<string>()->composing(), "table name")
  ("database-user", boost::program_options::value<string>()->composing(), "database user id")
  ("database-password", boost::program_options::value<string>()->composing(), "database password")
  ;
}

ChainPlugin::~ChainPlugin() {
  impl.reset();
}
} // namespace gruut
