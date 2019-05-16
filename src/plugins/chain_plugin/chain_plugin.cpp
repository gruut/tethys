#include "include/chain_plugin.hpp"

namespace gruut {

namespace fs = boost::filesystem;

using namespace std;

class TransactionMsgParser {
public:
  optional<Transaction> operator()(const nlohmann::json &transaction_message) {
    Transaction tx;

    bool result = tx.setJson(transaction_message);
    if (!result)
      return {};

    return tx;
  }
};

class TransactionMessageVerifier {
public:
  bool operator()(const Transaction &transaction) {
    if (!verifyID(transaction))
      return false;

    if (!verifyEndorserSig(transaction))
      return false;

    if (!verifyUserSig(transaction))
      return false;

    return true;
  }

private:
  bool verifyID(const Transaction &transaction) const {
    BytesBuilder tx_id_builder;
    tx_id_builder.appendBase<58>(transaction.getUserId());
    tx_id_builder.append(transaction.getWorld());
    tx_id_builder.append(transaction.getChain());
    tx_id_builder.appendDec(transaction.getTxTime());

    auto receiver_id = transaction.getReceiverId();
    receiver_id.empty() ? tx_id_builder.append("") : tx_id_builder.appendBase<58>(receiver_id);

    tx_id_builder.appendDec(transaction.getFee());
    tx_id_builder.append(transaction.getContractId());
    tx_id_builder.append(transaction.getTxInputCbor());

    vector<uint8_t> tx_id = Sha256::hash(tx_id_builder.getBytes());
    if (transaction.getTxId() != TypeConverter::encodeBase<58>(tx_id)) {
      return false;
    }

    return true;
  }

  bool verifyEndorserSig(const Transaction &transaction) const {
    AGS ags;

    auto &endorsers = transaction.getEndorsers();
    bool result = all_of(endorsers.begin(), endorsers.end(), [&ags, &transaction](const auto &endorser) {
      return ags.verify(endorser.endorser_pk, transaction.getTxId(), endorser.endorser_sig);
    });

    return result;
  }

  bool verifyUserSig(const Transaction &transaction) const {
    AGS ags;

    auto &endorsers = transaction.getEndorsers();
    string message = transaction.getTxId();
    for_each(endorsers.begin(), endorsers.end(),
             [&message](auto &endorser) { message += endorser.endorser_id + endorser.endorser_pk + endorser.endorser_sig; });

    auto user_sig = transaction.getUserSig();

    return ags.verify(transaction.getTxUserPk(), message, user_sig);
  }
};

class TransactionPool {
public:
  void add(const Transaction &tx) {
    {
      unique_lock<shared_mutex> writerLock(pool_mutex);

      tx_pool.try_emplace(tx.getTxId(), tx);
    }
  }

  vector<Transaction> fetchAll() {
    {
      shared_lock<shared_mutex> readerLock(pool_mutex);

      vector<Transaction> v;
      v.reserve(tx_pool.size());

      std::transform(tx_pool.begin(), tx_pool.end(), std::back_inserter(v), [](auto &kv) { return kv.second; });

      return v;
    }
  }

private:
  std::map<string, Transaction> tx_pool;
  std::shared_mutex pool_mutex;
};

class ChainPluginImpl {
public:
  unique_ptr<Chain> chain;
  unique_ptr<TransactionPool> transaction_pool;
  unique_ptr<UnresolvedBlockPool> unresolved_block_pool;

  string dbms;
  string table_name;
  string db_user_id;
  string db_password;

  string block_input_path;

  nlohmann::json genesis_state;

  incoming::channels::transaction::channel_type::Handle incoming_transaction_subscription;
  incoming::channels::block::channel_type::Handle incoming_block_subscription;

  void initialize() {
    chain = make_unique<Chain>(dbms, table_name, db_user_id, db_password);
    transaction_pool = make_unique<TransactionPool>();
    unresolved_block_pool = make_unique<UnresolvedBlockPool>();

    chain->startup(genesis_state);
  }

  void pushTransaction(const nlohmann::json &transaction_json) {
    logger::INFO("pushTransaction() Called");
    TransactionMsgParser parser;
    const auto transaction = parser(transaction_json);
    if (!transaction.has_value())
      return;

    TransactionMessageVerifier verfier;
    auto valid = verfier(transaction.value());

    if (valid) {
      transaction_pool->add(transaction.value());
    }
  }

  void pushBlock(const nlohmann::json &block_json) {
    logger::INFO("chain_plugin - pushBlock() Called");

    // TODO: 입력되는 블록 관련 메세지가 BONE이나 PING일 경우의 처리 추가 필요
    // TODO: block 구조체 형태로 입력되는게 맞을지 재고려

    Block input_block;
    input_block.initialize(block_json);

    ubp_push_result_type push_result = unresolved_block_pool->push(input_block);
    if (push_result.height == 0) {
      logger::ERROR("Block input fail: height 0");
      return;
    }
    if (push_result.duplicated) {
      logger::ERROR("Block input fail: duplicated");
      return;
    }

    UnresolvedBlock resolved_block;
    bool resolve_result = unresolved_block_pool->resolveBlock(input_block, resolved_block);

    if (resolve_result) {
      chain->insertBlockData(resolved_block.block);
    }

    return;
  }

  vector<Transaction> getTransactions() {
    return transaction_pool->fetchAll();
  }

  void start() {
    // TODO: msg 관련 요청 감시 (block, ping, request, etc..)

    { // test code start
      // 테스트 시에는 임의로 block_input_test.json의 블록들을 저장하는것부터 시작.
      unresolved_block_pool->setPool("11111111111111111111111111111111", 0, 0,
                                     "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA=", "11111111111111111111111111111111");

      nlohmann::json input_block_json;

      fs::path config_path = block_input_path;
      if (config_path.is_relative())
        config_path = fs::current_path() / config_path;

      std::ifstream i(config_path.make_preferred().string());

      i >> input_block_json;

      std::vector<Block> blocks;
      blocks.resize(5);

      for (int i = 0; i < 5; ++i) {
        blocks[i].initialize(input_block_json[i]);

        ubp_push_result_type push_result = unresolved_block_pool->push(blocks[i]);
        if (push_result.height == 0) {
          logger::ERROR("Block input fail: height 0");
          return;
        }
        if (push_result.duplicated) {
          logger::ERROR("Block input fail: duplicated");
          return;
        }

        UnresolvedBlock resolved_block;
        bool resolve_result = unresolved_block_pool->resolveBlock(blocks[i], resolved_block);
        if (resolve_result) {
          chain->insertBlockData(resolved_block.block);
        }
      }
    } // test code end

    { // test code start
      // 단순 입력 테스트. 실제로는 push해서 들어가야 한다.
      //    chain->insertBlockData(blocks[0]);
      //    logger::INFO("first block id: " + first_block.getBlockId());
      //    logger::INFO("first block 0th cert content: " + first_block.getUserCerts()[0].cert_content);
      //    logger::INFO("first block 0th txid: " + first_block.getTransactions()[0].getTxId());
      //    logger::INFO("first block 0th cid: " + first_block.getTransactions()[0].getContractId());
    } // test code end
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

  if (options.count("block-input-path")) {
    impl->block_input_path = options.at("block-input-path").as<string>();
  } else {
    throw std::invalid_argument("the input of block input path is empty"s);
  }

  auto &transaction_channel = app().getChannel<incoming::channels::transaction::channel_type>();
  impl->incoming_transaction_subscription =
      transaction_channel.subscribe([this](const nlohmann::json &transaction) { impl->pushTransaction(transaction); });

  auto &block_channel = app().getChannel<incoming::channels::block::channel_type>();
  impl->incoming_block_subscription = block_channel.subscribe([this](const nlohmann::json &block) { impl->pushBlock(block); });

  impl->initialize();
}

// clang-format off
void ChainPlugin::setProgramOptions(options_description &cfg) {
  cfg.add_options()("world-create", boost::program_options::value<string>()->composing(), "the location of a world_create.json file")
  ("dbms", boost::program_options::value<string>()->composing(), "DBMS (MYSQL)")("table-name", boost::program_options::value<string>()->composing(), "table name")
  ("database-user", boost::program_options::value<string>()->composing(), "database user id")
  ("database-password", boost::program_options::value<string>()->composing(), "database password")
  ("block-input-path", boost::program_options::value<string>()->composing(), "block input json path");
}
// clang-format on

void ChainPlugin::asyncFetchTransactionsFromPool() {
  auto transactions = impl->getTransactions();
  app().getChannel<incoming::channels::transaction_pool::channel_type>().publish(transactions);
}

void ChainPlugin::pluginStart() {
  logger::INFO("ChainPlugin Start");

  impl->start();
}

ChainPlugin::~ChainPlugin() {
  impl.reset();
}
} // namespace gruut
