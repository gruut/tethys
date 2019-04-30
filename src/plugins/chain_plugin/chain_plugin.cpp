#include "include/chain_plugin.hpp"
#include "../../../include/json.hpp"
#include "../../../include/types/transaction.hpp"
#include "../../../lib/appbase/include/application.hpp"
#include "../../../lib/log/include/log.hpp"
#include "include/chain.hpp"

namespace gruut {

namespace fs = boost::filesystem;

using namespace std;

class TransactionMsgParser {
public:
  TransactionMessage operator()(const nlohmann::json &transaction_message) {
    TransactionMessage tx_message;

    tx_message.txid = transaction_message["/txid"_json_pointer];
    tx_message.world = transaction_message["/world"_json_pointer];
    tx_message.chain = transaction_message["/chain"_json_pointer];
    tx_message.time = transaction_message["/time"_json_pointer];

    tx_message.cid = transaction_message["/body/cid"_json_pointer];
    tx_message.receiver = transaction_message["/body/receiver"_json_pointer];
    tx_message.fee = transaction_message["/body/fee"_json_pointer];

    tx_message.user_id = transaction_message["/user/id"_json_pointer];
    tx_message.user_pk = transaction_message["/user/pk"_json_pointer];
    tx_message.user_sig = transaction_message["/user/sig"_json_pointer];

    tx_message.endorser_id = transaction_message["/endorser/id"_json_pointer];
    tx_message.endorser_pk = transaction_message["/endorser/pk"_json_pointer];
    tx_message.endorser_sig = transaction_message["/endorser/sig"_json_pointer];

    return tx_message;
  }
};

class ChainPluginImpl {
public:
  unique_ptr<Chain> chain;

  string dbms;
  string table_name;
  string db_user_id;
  string db_password;

  nlohmann::json genesis_state;

  incoming::channels::transaction::channel_type::Handle incoming_transaction_subscription;

  void initialize() {
    chain = make_unique<Chain>(dbms, table_name, db_user_id, db_password);
    chain->startup(genesis_state);
  }

  void push_transaction(const nlohmann::json &transaction_json) {
    logger::INFO("Do something");
    // TODO: transaction 처리
    TransactionMsgParser parser;
    auto transaction = parser(transaction_json);
    // TODO
    // STEP 1, Verify
    //    auto valid = verifyTransaction(transaction);
    // STEP 2, If the transaction is valid, push it into transaction pool.
    //    if (valid) {
    //      transaction_pool.push(transaction);
    //    }
  }
};

ChainPlugin::ChainPlugin() : impl(make_unique<ChainPluginImpl>()) {}

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

  auto &transaction_channel = app().getChannel<incoming::channels::transaction::channel_type>();
  impl->incoming_transaction_subscription =
      transaction_channel.subscribe([this](nlohmann::json transaction) { impl->push_transaction(transaction); });

  impl->initialize();
}

// clang-format off
void ChainPlugin::setProgramOptions(options_description &cfg) {
  cfg.add_options()("genesis-block", boost::program_options::value<string>()->composing(), "the location of a genesis_block.json file")
  ("dbms", boost::program_options::value<string>()->composing(), "DBMS (MYSQL)")("table-name", boost::program_options::value<string>()->composing(), "table name")
  ("database-user", boost::program_options::value<string>()->composing(), "database user id")
  ("database-password", boost::program_options::value<string>()->composing(), "database password");
}
// clang-format on

ChainPlugin::~ChainPlugin() {
  impl.reset();
}
} // namespace gruut
