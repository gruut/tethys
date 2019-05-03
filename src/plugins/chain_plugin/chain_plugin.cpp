#include "include/chain.hpp"
#include "include/chain_plugin.hpp"
#include "../../../include/json.hpp"
#include "../../../include/types/transaction.hpp"
#include "../../../lib/appbase/include/application.hpp"
#include "../../../lib/gruut-utils/src/bytes_builder.hpp"
#include "../../../lib/gruut-utils/src/sha256.hpp"
#include "../../../lib/gruut-utils/src/type_converter.hpp"
#include "../../../lib/log/include/log.hpp"
#include "../../../lib/core/include/transaction_pool.hpp"
#include "structure/block.hpp"

namespace gruut {

namespace fs = boost::filesystem;

using namespace std;
using namespace core;

class TransactionMsgParser {
public:
  optional<TransactionMessage> operator()(const nlohmann::json &transaction_message) {
    try {
      TransactionMessage tx_message;

      tx_message.txid = transaction_message["/txid"_json_pointer];
      tx_message.world = transaction_message["/world"_json_pointer];
      tx_message.chain = transaction_message["/chain"_json_pointer];
      tx_message.time = transaction_message["/time"_json_pointer];

      tx_message.cid = transaction_message["/body/cid"_json_pointer];
      tx_message.receiver = transaction_message["/body/receiver"_json_pointer];
      tx_message.fee = transaction_message["/body/fee"_json_pointer];
      tx_message.input = nlohmann::json::to_cbor(transaction_message["body"]["input"]);

      tx_message.user_id = transaction_message["/user/id"_json_pointer];
      tx_message.user_pk = transaction_message["/user/pk"_json_pointer];
      tx_message.user_sig = transaction_message["/user/sig"_json_pointer];

      tx_message.endorser_id = transaction_message["/endorser/id"_json_pointer];
      tx_message.endorser_pk = transaction_message["/endorser/pk"_json_pointer];
      tx_message.endorser_sig = transaction_message["/endorser/sig"_json_pointer];

      return tx_message;
    } catch (nlohmann::json::parse_error &e) {
      logger::ERROR("Failed to parse MSG_TX: {}", e.what());
      return {};

    }
  }
};

class TransactionMessageVerifier {
public:
  bool operator()(const TransactionMessage &transaction_message) {
    BytesBuilder tx_id_builder;
    tx_id_builder.appendBase<58>(transaction_message.user_id);
    tx_id_builder.append(transaction_message.world);
    tx_id_builder.append(transaction_message.chain);
    tx_id_builder.appendDec(transaction_message.time);

    transaction_message.receiver.empty() ? tx_id_builder.append("") : tx_id_builder.appendBase<58>(transaction_message.receiver);

    tx_id_builder.appendDec(transaction_message.fee);
    tx_id_builder.append(transaction_message.cid);
    tx_id_builder.append(transaction_message.input);

    vector<uint8_t> tx_id = Sha256::hash(tx_id_builder.getBytes());
    if (transaction_message.txid != TypeConverter::encodeBase<58>(tx_id)) {
      return false;
    }

    // TODO: SignByUser, SignByEndorser 검증 구현
    return true;
  }
};

class ChainPluginImpl {
public:
  unique_ptr<Chain> chain;
  shared_ptr<DBController> rdb_controller;
  unique_ptr<TransactionPool> transaction_pool;

  string dbms;
  string table_name;
  string db_user_id;
  string db_password;

  nlohmann::json genesis_state;

  incoming::channels::transaction::channel_type::Handle incoming_transaction_subscription;

  void initialize() {
    chain = make_unique<Chain>();
    transaction_pool = make_unique<TransactionPool>();

    chain->startup(genesis_state);

    rdb_controller = make_shared<DBController>(dbms, table_name, db_user_id, db_password);
  }

  void pushTransaction(const nlohmann::json &transaction_json) {
    TransactionMsgParser parser;
    const auto transaction_message = parser(transaction_json);
    if(!transaction_message.has_value())
      return;

    TransactionMessageVerifier verfier;
    auto valid = verfier(transaction_message.value());

    if (valid) {
      transaction_pool->add(transaction_message.value());
    }
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
      transaction_channel.subscribe([this](nlohmann::json transaction) { impl->pushTransaction(transaction); });

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
