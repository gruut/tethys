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
  base58_type uid = json::get<string>(option, "uid").value();
  string gender = json::get<string>(option, "gender").value();
  int age = stoi(json::get<string>(option, "age").value());
  string isc_type = json::get<string>(option, "isc_type").value();
  string isc_code = json::get<string>(option, "isc_code").value();
  string location = json::get<string>(option, "location").value();

  // TODO: 2.1.2. User Attributes 테이블에 추가하는 코드 작성
  return true;
}

bool Chain::queryUserCert(UnresolvedBlock &UR_block, nlohmann::json &option, result_query_info_type &result_info) {
  base58_type uid = json::get<string>(option, "uid").value();
  timestamp_t notbefore = static_cast<uint64_t>(stoll(json::get<string>(option, "notbefore").value()));
  timestamp_t notafter = static_cast<uint64_t>(stoll(json::get<string>(option, "notafter").value()));
  string sn = json::get<string>(option, "sn").value();
  string x509 = json::get<string>(option, "x509").value();

  // TODO: 2.1.1. User Certificates 테이블에 추가하는 코드 작성
  return true;
}

bool Chain::queryContractNew(UnresolvedBlock &UR_block, nlohmann::json &option, result_query_info_type &result_info) {
  contract_id_type cid = json::get<string>(option, "cid").value();
  timestamp_t after = static_cast<uint64_t>(stoll(json::get<string>(option, "after").value()));
  timestamp_t before = static_cast<uint64_t>(stoll(json::get<string>(option, "before").value()));
  base58_type author = json::get<string>(option, "author").value();
  contract_id_type friend_id = json::get<string>(option, "friend").value();
  string contract_xml = json::get<string>(option, "contract").value();
  string desc = json::get<string>(option, "desc").value();
  string sigma = json::get<string>(option, "sigma").value();

  // TODO: db에 접근하여 contract 갱신. friend 추가할 때 자신을 friend로 추가한 contract를 찾아서 추가해야함을 주의
  return true;
}

bool Chain::queryContractDisable(UnresolvedBlock &UR_block, nlohmann::json &option, result_query_info_type &result_info) {
  contract_id_type cid = json::get<string>(option, "cid").value();

  // TODO: db에 접근하여 contract 갱신
  return true;
}

bool Chain::queryIncinerate(UnresolvedBlock &UR_block, nlohmann::json &option, result_query_info_type &result_info) {
  int amount = stoi(json::get<string>(option, "amount").value());
  string pid = json::get<string>(option, "pid").value();

  // TODO: m_mem_ledger 사용하여 갱신값 계산
  return true;
}

bool Chain::queryCreate(UnresolvedBlock &UR_block, nlohmann::json &option, result_query_info_type &result_info) {
  int amount = stoi(json::get<string>(option, "amount").value());
  string name = json::get<string>(option, "name").value();
  string type = json::get<string>(option, "type").value();
  string tag = json::get<string>(option, "tag").value();

  // TODO: m_mem_ledger 사용하여 갱신값 계산
  return true;
}

bool Chain::queryTransfer(UnresolvedBlock &UR_block, nlohmann::json &option, result_query_info_type &result_info) {
  base58_type from = json::get<string>(option, "from").value();
  base58_type to = json::get<string>(option, "to").value();
  int amount = stoi(json::get<string>(option, "amount").value());
  string unit = json::get<string>(option, "unit").value();
  string pid = json::get<string>(option, "pid").value();
  string tag = json::get<string>(option, "tag").value();

  // TODO: m_mem_ledger 사용하여 갱신값 계산
  //    from과 to가 user/contract 따라 처리될 수 있도록 구현 필요
  return true;
}

bool Chain::queryUserScope(UnresolvedBlock &UR_block, nlohmann::json &option, result_query_info_type &result_info) {
  string name = json::get<string>(option, "name").value();
  string value = json::get<string>(option, "value").value();
  base58_type uid = json::get<string>(option, "uid").value();
  string pid = json::get<string>(option, "pid").value();
  string tag = json::get<string>(option, "tag").value();

  // TODO: m_mem_ledger 사용하여 갱신값 계산
  return true;
}

bool Chain::queryContractScope(UnresolvedBlock &UR_block, nlohmann::json &option, result_query_info_type &result_info) {
  string name = json::get<string>(option, "name").value();
  string value = json::get<string>(option, "value").value();
  contract_id_type cid = json::get<string>(option, "cid").value();
  string pid = json::get<string>(option, "pid").value();

  // TODO: m_mem_ledger 사용하여 갱신값 계산
  return true;
}

bool Chain::queryTradeItem(UnresolvedBlock &UR_block, nlohmann::json &option, result_query_info_type &result_info) {
  nlohmann::json costs = option["costs"];
  nlohmann::json units = option["units"];
  base58_type author = json::get<string>(option, "author").value();
  base58_type user = json::get<string>(option, "user").value();
  string pid = json::get<string>(option, "pid").value();

  // TODO: 처리 절차 구현할 것
  return true;
}

bool Chain::queryTradeVal(UnresolvedBlock &UR_block, nlohmann::json &option, result_query_info_type &result_info) {
  // TODO: TBA (19.05.21 현재 문서에 관련 사항 미기재)
  return true;
}

bool Chain::queryRunQuery(UnresolvedBlock &UR_block, nlohmann::json &option, result_query_info_type &result_info) {
  string type = json::get<string>(option, "type").value();
  nlohmann::json query = option["query"];
  timestamp_t after = static_cast<uint64_t>(stoll(json::get<string>(option, "after").value()));

  // TODO: Scheduler에게 지연 처리 요청 전송
  return true;
}

bool Chain::queryRunContract(UnresolvedBlock &UR_block, nlohmann::json &option, result_query_info_type &result_info) {
  contract_id_type cid = json::get<string>(option, "cid").value();
  string input = json::get<string>(option, "input").value();
  timestamp_t after = static_cast<uint64_t>(stoll(json::get<string>(option, "after").value()));

  // TODO: authority.user를 현재 user로 대체하여 Scheduler에게 요청 전송
  return true;
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
