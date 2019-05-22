#include "include/chain.hpp"

namespace gruut {

class ChainImpl {
public:
  ChainImpl(Chain &self) : self(self) {}

  void init(nlohmann::json genesis_state) {
    world_type genesis = unmarshalGenesisState(genesis_state);

    self.saveWorld(genesis);
    self.saveChain(genesis.local_chain_state);

    string tmp_key_test = genesis.world_id + "_cpk";
    string test_value = self.getValueByKey(DataType::WORLD, tmp_key_test);

    logger::INFO("KV levelDB test... " + test_value);
  }

  world_type unmarshalGenesisState(nlohmann::json state) {
    try {
      world_type genesis_state;

      genesis_state.world_id = state["/world/id"_json_pointer];
      genesis_state.world_created_time = state["/world/after"_json_pointer];

      genesis_state.keyc_name = state["/key_currency/name"_json_pointer];
      genesis_state.initial_amount = state["/key_currency/initial_amount"_json_pointer];

      genesis_state.allow_mining = state["/mining_policy/allow_mining"_json_pointer];

      genesis_state.allow_anonymous_user = state["/user_policy/allow_anonymous_user"_json_pointer];
      genesis_state.join_fee = state["/user_policy/join_fee"_json_pointer];

      genesis_state.local_chain_state.chain_id = state["/eden/chain/id"_json_pointer];
      genesis_state.local_chain_state.world_id = state["/eden/chain/world"_json_pointer];
      genesis_state.local_chain_state.chain_created_time = state["/eden/chain/after"_json_pointer];

      genesis_state.local_chain_state.allow_custom_contract = state["/eden/policy/allow_custom_contract"_json_pointer];
      genesis_state.local_chain_state.allow_oracle = state["/eden/policy/allow_oracle"_json_pointer];
      genesis_state.local_chain_state.allow_tag = state["/eden/policy/allow_tag"_json_pointer];
      genesis_state.local_chain_state.allow_heavy_contract = state["/eden/policy/allow_heavy_contract"_json_pointer];

      genesis_state.authority_id = state["/authority/id"_json_pointer];
      genesis_state.authority_cert = state["authority"]["cert"].get<vector<string>>();

      genesis_state.creator_id = state["/creator/id"_json_pointer];
      genesis_state.local_chain_state.creator_id = state["/creator/id"_json_pointer];
      genesis_state.creator_cert = state["creator"]["cert"].get<vector<string>>();
      genesis_state.local_chain_state.creator_cert = state["creator"]["cert"].get<vector<string>>();
      genesis_state.creator_sig = state["/creator/sig"_json_pointer];
      genesis_state.local_chain_state.creator_sig = state["/creator/sig"_json_pointer];

      assert(genesis_state.world_id == genesis_state.local_chain_state.world_id);
      assert(genesis_state.creator_id == genesis_state.local_chain_state.creator_id);
      assert(genesis_state.creator_cert == genesis_state.local_chain_state.creator_cert);
      assert(genesis_state.creator_sig == genesis_state.local_chain_state.creator_sig);

      return genesis_state;
    } catch (nlohmann::json::parse_error &e) {
      logger::ERROR("Failed to parse world_create.json: {}", e.what());
      throw e;
    }
  }

  Chain &self;
};

Chain::Chain(string_view dbms, string_view table_name, string_view db_user_id, string_view db_password) {
  impl = make_unique<ChainImpl>(*this);
  rdb_controller = make_unique<RdbController>(dbms, table_name, db_user_id, db_password);
  kv_controller = make_unique<KvController>();
}

void Chain::startup(nlohmann::json &genesis_state) {
  impl->init(genesis_state);
}

Chain::~Chain() {
  impl.reset();
}

// RDB functions
void Chain::insertBlockData(gruut::Block &block_info) {
  rdb_controller->insertBlockData(block_info);
}

void Chain::insertTransactionData(gruut::Block &block_info) {
  rdb_controller->insertTransactionData(block_info);
}

vector<Block> Chain::getBlocksByHeight(int from, int to) {
  if (from > to) {
    return vector<Block>();
  }

  stringstream ss;
  ss << "block_height BETWEEN " << from << " AND " << to;

  vector<Block> blocks = rdb_controller->getBlocks(ss.str());
  return blocks;
}

block_height_type Chain::getLatestResolvedHeight() {
  string condition = "ORDER BY block_height DESC LIMIT 1";
  Block block = rdb_controller->getBlock(condition);

  return block.getHeight();
}

string Chain::getUserCert(const base58_type &user_id) {
  return rdb_controller->getUserCert(user_id);
}

bool Chain::queryUserJoin(UnresolvedBlock &UR_block, nlohmann::json &option, result_query_info_type &result_info) {
  return rdb_controller->queryUserJoin(UR_block, option, result_info);
}

bool Chain::queryUserCert(UnresolvedBlock &UR_block, nlohmann::json &option, result_query_info_type &result_info) {
  return rdb_controller->queryUserCert(UR_block, option, result_info);
}

bool Chain::queryContractNew(UnresolvedBlock &UR_block, nlohmann::json &option, result_query_info_type &result_info) {
  return rdb_controller->queryContractNew(UR_block, option, result_info);
}

bool Chain::queryContractDisable(UnresolvedBlock &UR_block, nlohmann::json &option, result_query_info_type &result_info) {
  return rdb_controller->queryContractDisable(UR_block, option, result_info);
}

bool Chain::queryIncinerate(UnresolvedBlock &UR_block, nlohmann::json &option, result_query_info_type &result_info) {
  return rdb_controller->queryIncinerate(UR_block, option, result_info);
}

bool Chain::queryCreate(UnresolvedBlock &UR_block, nlohmann::json &option, result_query_info_type &result_info) {
  return rdb_controller->queryCreate(UR_block, option, result_info);
}

bool Chain::queryTransfer(UnresolvedBlock &UR_block, nlohmann::json &option, result_query_info_type &result_info) {
  return rdb_controller->queryTransfer(UR_block, option, result_info);
}

bool Chain::queryUserScope(UnresolvedBlock &UR_block, nlohmann::json &option, result_query_info_type &result_info) {
  return rdb_controller->queryUserScope(UR_block, option, result_info);
}

bool Chain::queryContractScope(UnresolvedBlock &UR_block, nlohmann::json &option, result_query_info_type &result_info) {
  return rdb_controller->queryContractScope(UR_block, option, result_info);
}

bool Chain::queryTradeItem(UnresolvedBlock &UR_block, nlohmann::json &option, result_query_info_type &result_info) {
  return rdb_controller->queryTradeItem(UR_block, option, result_info);
}

bool Chain::queryTradeVal(UnresolvedBlock &UR_block, nlohmann::json &option, result_query_info_type &result_info) {
  return rdb_controller->queryTradeVal(UR_block, option, result_info);
}

bool Chain::queryRunQuery(UnresolvedBlock &UR_block, nlohmann::json &option, result_query_info_type &result_info) {
  return rdb_controller->queryRunQuery(UR_block, option, result_info);
}

bool Chain::queryRunContract(UnresolvedBlock &UR_block, nlohmann::json &option, result_query_info_type &result_info) {
  return rdb_controller->queryRunContract(UR_block, option, result_info);
}

// KV functions
void Chain::saveWorld(world_type &world_info) {
  kv_controller->saveWorld(world_info);
}

void Chain::saveChain(local_chain_type &chain_info) {
  kv_controller->saveChain(chain_info);
}

void Chain::saveBackup(UnresolvedBlock &block_info) {
  kv_controller->saveBackup(block_info);
}

void Chain::saveSelfInfo(self_info_type &self_info) {
  kv_controller->saveSelfInfo(self_info);
}

string Chain::getValueByKey(DataType what, const string &base_keys) {
  return kv_controller->getValueByKey(what, base_keys);
}

} // namespace gruut
