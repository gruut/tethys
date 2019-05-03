#include "include/chain.hpp"
#include "include/chain_plugin.hpp"
#include "../../../include/json.hpp"
#include "../../../lib/appbase/include/application.hpp"
#include "../../../lib/log/include/log.hpp"
#include "structure/block.hpp"

namespace gruut {

namespace fs = boost::filesystem;

using namespace std;

class ChainPluginImpl {
public:
  unique_ptr<Chain> chain;
  shared_ptr<DBController> rdb_controller;

  string dbms;
  string table_name;
  string db_user_id;
  string db_password;

  nlohmann::json genesis_state;

  incoming::channels::transaction::channel_type::Handle incoming_transaction_subscription;

  void initialize() {
    chain = make_unique<Chain>();
    chain->startup(genesis_state);

    rdb_controller = make_shared<DBController>(dbms, table_name, db_user_id, db_password);
  }

  void push_transaction(nlohmann::json transaction) {
    logger::INFO("Do something");
  }

  void start() {
    // TODO: msg 관련 요청 감시 (block, ping, request, etc..)

    // 테스트 시에는 임의로 first_block_test.json의 블록 하나를 저장하는것부터 시작.
    nlohmann::json first_block_json;

    fs::path config_path = "../first_block_test.json";
    if (config_path.is_relative())
      config_path = fs::current_path() / config_path;

    std::ifstream i(config_path.make_preferred().string());

    i >> first_block_json;

    Block first_block;
    first_block.initialize(first_block_json);

    cout<<first_block.getBlockId()<<endl;
    cout<<first_block.getUserCerts()[0].cert_content<<endl;
    // end test code
  }
};

ChainPlugin::ChainPlugin() : impl(make_unique<ChainPluginImpl>()) {}

void ChainPlugin::pluginInitialize(const boost::program_options::variables_map &options) {
  logger::INFO("ChainPlugin Initialize");

  if (options.count("world-create") > 0) {
    fs::path config_path = options.at("world-create").as<std::string>();

    if (config_path.is_relative())
      config_path = fs::current_path() / config_path;

    if (!fs::exists(config_path)) {
      logger::ERROR("Can't find a world create file. (path: {})", config_path.string());
      throw std::invalid_argument("Cannot find a world_create.json"s);
    }

    std::ifstream i(config_path.make_preferred().string());

    i >> impl->genesis_state;
  } else {
    throw std::invalid_argument("world create file does not exist"s);
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

  auto &transaction_channel = app().getChannel<incoming::channels::transaction::channel_type>();
  impl->incoming_transaction_subscription =
      transaction_channel.subscribe([this](nlohmann::json transaction) { impl->push_transaction(transaction); });

  impl->initialize();
}

// clang-format off
void ChainPlugin::setProgramOptions(options_description &cfg) {
  cfg.add_options()("world-create", boost::program_options::value<string>()->composing(), "the location of a world_create.json file")
  ("dbms", boost::program_options::value<string>()->composing(), "DBMS (MYSQL)")("table-name", boost::program_options::value<string>()->composing(), "table name")
  ("database-user", boost::program_options::value<string>()->composing(), "database user id")
  ("database-password", boost::program_options::value<string>()->composing(), "database password");
}
// clang-format on

void ChainPlugin::pluginStart() {
  logger::INFO("ChainPlugin Start");

  impl->start();
}

ChainPlugin::~ChainPlugin() {
  impl.reset();
}
} // namespace gruut
