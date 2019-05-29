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
  unresolved_block_pool = make_unique<UnresolvedBlockPool>();
}

void Chain::startup(nlohmann::json &genesis_state) {
  impl->init(genesis_state);
}

Chain::~Chain() {
  impl.reset();
}

// RDB functions
string Chain::getUserCert(const base58_type &user_id) {
  return rdb_controller->getUserCert(user_id);
}

void Chain::insertBlockData(gruut::Block &block_info) {
  rdb_controller->insertBlockData(block_info);
}

void Chain::insertTransactionData(gruut::Block &block_info) {
  rdb_controller->insertTransactionData(block_info);
}

void Chain::insertUserLedgerData(std::map<string, user_ledger_type> &user_ledger) {
  rdb_controller->insertUserLedgerData(user_ledger);
}

void Chain::insertContractLedgerData(std::map<string, contract_ledger_type> &contract_ledger) {
  rdb_controller->insertContractLedgerData(contract_ledger);
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

// Unresolved block pool functions
bool Chain::queryIncinerate(UnresolvedBlock &UR_block, nlohmann::json &option, result_query_info_type &result_info) {
  string amount = json::get<string>(option, "amount").value();
  string pid = json::get<string>(option, "pid").value();

  user_ledger_type found = unresolved_block_pool->findUserLedgerFromHead(pid);
  if (found.uid != result_info.user)
    return false;
  int modified_value = stoi(found.var_val) - stoi(amount);

  user_ledger_type user_ledger;
  user_ledger.pid = TypeConverter::stringToBytes(pid);
  user_ledger.var_val = to_string(modified_value);

  if (modified_value <= 0)
    user_ledger.query_type = QueryType::DELETE;
  else
    user_ledger.query_type = QueryType::UPDATE;

  UR_block.user_ledger[pid] = user_ledger;

  return true;
}

bool Chain::queryCreate(UnresolvedBlock &UR_block, nlohmann::json &option, result_query_info_type &result_info) {
  string amount = json::get<string>(option, "amount").value();
  string var_name = json::get<string>(option, "name").value();
  int var_type = stoi(json::get<string>(option, "type").value());
  string tag = json::get<string>(option, "tag").value();
  base58_type uid = result_info.user;
  timestamp_t up_time = TimeUtil::nowBigInt(); // TODO: DB에 저장되는 시간인지, mem_ledger에 들어오는 시간인지
  block_height_type up_block = result_info.block_height;

  user_ledger_type user_ledger(var_name, var_type, uid, tag);
  string pid = TypeConverter::bytesToString(user_ledger.pid);

  user_ledger_type found = unresolved_block_pool->findUserLedgerFromHead(pid);
  if (found.uid != result_info.user)
    return false;

  int modified_value = stoi(found.var_val) + stoi(amount);
  user_ledger.var_val = to_string(modified_value);

  user_ledger.up_time = up_time;
  user_ledger.up_block = up_block;
  user_ledger.query_type = QueryType::INSERT;

  UR_block.user_ledger[pid] = user_ledger;

  return true;
}

bool Chain::queryTransfer(UnresolvedBlock &UR_block, nlohmann::json &option, result_query_info_type &result_info) {
  // TODO: 권한 체크 세부적으로 구현
  string from = json::get<string>(option, "from").value();
  string to = json::get<string>(option, "to").value();
  string amount = json::get<string>(option, "amount").value();
  string unit = json::get<string>(option, "unit").value();
  string key;
  auto pid = json::get<string>(option, "pid");
  string tag = json::get<string>(option, "tag").value();
  int var_type = loadVarType();

  // Transfer : from
  if (from == "contract") {
    // from : contract
    string cid = result_info.self;
    contract_ledger_type contract_ledger;

    key = pidCheck(pid, unit, var_type, from);
    if (pid.has_value()) {
      contract_ledger.pid = TypeConverter::stringToBytes(pid.value());
    }

    contract_ledger_type found = unresolved_block_pool->findContractLedgerFromHead(key);
    int modified_value = stoi(found.var_val) - stoi(amount);
    contract_ledger.var_val = to_string(modified_value);

    if (modified_value == 0)
      contract_ledger.query_type = QueryType::DELETE;
    else if (modified_value > 0)
      contract_ledger.query_type = QueryType::UPDATE;
    else {
      logger::ERROR("var_val is under 0 in queryTransfer!");
      return false;
    }

    UR_block.contract_ledger[key] = contract_ledger;
  } else {
    // from : user
    base58_type uid;
    if (from == "user")
      uid = result_info.user;
    else if (from == "author")
      uid = result_info.author;

    user_ledger_type user_ledger(unit, var_type, uid, tag);

    key = pidCheck(pid, unit, var_type, uid);
    if (pid.has_value()) {
      user_ledger.pid = TypeConverter::stringToBytes(pid.value());
    }

    user_ledger_type found = unresolved_block_pool->findUserLedgerFromHead(key);
    int modified_value = stoi(found.var_val) - stoi(amount);
    user_ledger.var_val = to_string(modified_value);

    if (modified_value == 0)
      user_ledger.query_type = QueryType::DELETE;
    else if (modified_value > 0)
      user_ledger.query_type = QueryType::UPDATE;
    else {
      logger::ERROR("var_val is under 0 in queryTransfer!");
      return false;
    }

    UR_block.user_ledger[key] = user_ledger;
  }

  // Transfer : to
  if (to.size() > 45) {
    // to : contract
    contract_ledger_type contract_ledger;

    key = pidCheck(pid, unit, var_type, to);
    if (pid.has_value()) {
      contract_ledger.pid = TypeConverter::stringToBytes(pid.value());
    }

    contract_ledger_type found = unresolved_block_pool->findContractLedgerFromHead(key);

    if (found.var_val == "")
      contract_ledger.query_type = QueryType::INSERT;
    else
      contract_ledger.query_type = QueryType::UPDATE;

    int modified_value = stoi(found.var_val) + stoi(amount);
    contract_ledger.var_val = to_string(modified_value);
    contract_ledger.var_info = from;

    UR_block.contract_ledger[key] = contract_ledger;
  } else {
    // to : user
    user_ledger_type user_ledger;

    key = pidCheck(pid, unit, var_type, to);
    if (pid.has_value()) {
      user_ledger.pid = TypeConverter::stringToBytes(pid.value());
    }

    user_ledger_type found = unresolved_block_pool->findUserLedgerFromHead(key);
    if (found.uid != result_info.user)
      return false;

    if (found.var_val == "")
      user_ledger.query_type = QueryType::INSERT;
    else
      user_ledger.query_type = QueryType::UPDATE;

    int modified_value = stoi(found.var_val) + stoi(amount);
    user_ledger.var_val = to_string(modified_value);

    user_ledger.tag = tag;

    UR_block.user_ledger[key] = user_ledger;
  }

  return true;
}

bool Chain::queryUserScope(UnresolvedBlock &UR_block, nlohmann::json &option, result_query_info_type &result_info) {
  user_ledger_type user_ledger;

  string var_name = json::get<string>(option, "name").value();
  string target = json::get<string>(option, "for").value(); // user or author
  string var_value = json::get<string>(option, "value").value();
  int var_type; // TODO: 세부적인 값 설정 필요
  string key;
  auto pid = json::get<string>(option, "pid");
  if (pid.has_value()) {
    user_ledger.pid = TypeConverter::stringToBytes(pid.value());
  } else {
    // TODO: var_name이 unique한지 검증하는 함수 추가
    //  var_name이 없으면 insert, 1개면 update, 2개 이상이면 false
    //  insert일 경우 var_type이 존재, 다른 경우 var_type가 생략
    var_type = -1;
  }
  string tag = json::get<string>(option, "tag").value();
  auto type_json = json::get<string>(option, "type");
  if (type_json.has_value()) {
    user_ledger.query_type = QueryType::INSERT;
    var_type = stoi(type_json.value());
  } else
    user_ledger.query_type = QueryType::UPDATE;

  base58_type uid;
  if (target == "user") {
    uid = result_info.user;
  } else if (target == "author") {
    uid = result_info.author;
  } else {
    logger::ERROR("target is not 'user' or 'author' at `queryUserScope`");
  }

  key = pidCheck(pid, var_name, var_type, uid);

  // TODO: '통화'속성의 변수는 변경 불가능한 조건 검사 시행

  user_ledger.var_val = var_value;
  user_ledger.tag = tag; // TODO: 변수가 존재하는 경우 무시

  UR_block.user_ledger[key] = user_ledger;

  return true;
}

bool Chain::queryContractScope(UnresolvedBlock &UR_block, nlohmann::json &option, result_query_info_type &result_info) {
  contract_ledger_type contract_ledger;

  string var_name = json::get<string>(option, "name").value();
  string var_value = json::get<string>(option, "value").value();
  int var_type; // TODO: 세부적인 값 설정 필요
  contract_id_type cid = json::get<string>(option, "cid").value();
  string key;
  auto pid = json::get<string>(option, "pid");
  if (pid.has_value()) {
    key = pid.value();
  } else {
    // TODO: var_name이 unique한지 검증하는 함수 추가
    //  var_name이 없으면 insert, 1개면 update, 2개 이상이면 false
    //  insert일 경우 var_type이 존재, 다른 경우 var_type가 생략
    var_type = -1;
  }
  auto type_json = json::get<string>(option, "type");
  if (type_json.has_value()) {
    contract_ledger.query_type = QueryType::INSERT;
    var_type = stoi(type_json.value());
  } else
    contract_ledger.query_type = QueryType::UPDATE;

  key = pidCheck(pid, var_name, var_type, cid);

  // TODO: '통화'속성의 변수는 변경 불가능한 조건 검사 시행

  contract_ledger.var_val = var_value;

  UR_block.contract_ledger[key] = contract_ledger;

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

  if ((type == "run.query") || (type == "user.cert")) {
    logger::ERROR("run.query cannot excute 'run.query' or 'user.cert'!");
    return false;
  }
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

string Chain::pidCheck(optional<string> pid, string var_name, int var_type, string var_owner) {
  string key;
  if (pid.has_value()) {
    key = pid.value();
  } else {
    // TODO: pid 없이도 unique함이 확인되어야 함
    BytesBuilder bytes_builder;
    bytes_builder.append(var_name);
    bytes_builder.append(var_type);
    bytes_builder.append(var_owner);
    key = TypeConverter::bytesToString(Sha256::hash(bytes_builder.getBytes()));
  }

  return key;
}

user_ledger_type Chain::findUserLedgerFromPoint(string key, int deq_idx, int vec_idx) {
  int pool_deque_idx = find_start_height - unresolved_block_pool->getLatestConfirmedHeight() - 1;
  int pool_vec_idx = vec_idx;

  map<string, user_ledger_type> current_user_ledgers = unresolved_block_pool->getBlock(pool_deque_idx, pool_vec_idx).user_ledger;
  map<string, user_ledger_type>::iterator it;

  while (1) {
    it = current_user_ledgers.find(key);

    if (it != current_user_ledgers.end()) {
      if (it->second.is_deleted) {
        return;
      } else {
        return;
      }
      break;
    }
    --pool_deque_idx;
    pool_vec_idx = unresolved_block_pool->getBlock(pool_deque_idx, pool_vec_idx).prev_vector_idx;

    if (pool_deque_idx < 0) {
      // -1이 되면 db resolved 층까지 왔다는 이야기이다
      // db에서 select문으로 조회하는데도 찾지 못한다면, 그것은 아예 존재하지 않는 데이터
      return false;
    }

    current_user_ledgers = unresolved_block_pool->getBlock(pool_deque_idx, pool_vec_idx).user_ledger;
  }

  return unresolved_block_pool->getBlock(pool_deque_idx, pool_vec_idx).user_ledger[key];
}

ubp_push_result_type Chain::pushBlock(Block &block, bool is_restore) {
  return unresolved_block_pool->pushBlock(block, is_restore);
}

UnresolvedBlock Chain::findBlock(const base58_type &block_id, const block_height_type block_height) {
  return unresolved_block_pool->findBlock(block_id, block_height);
}

bool Chain::resolveBlock(Block &block, UnresolvedBlock &resolved_result) {
  return unresolved_block_pool->resolveBlock(block, resolved_result);
}

base58_type Chain::getCurrentHeadId() {
  return unresolved_block_pool->getCurrentHeadId();
}

bytes Chain::getUserStateRoot() {
  return unresolved_block_pool->getUserStateRoot();
}

bytes Chain::getContractStateRoot() {
  return unresolved_block_pool->getContractStateRoot();
}

void Chain::setPool(const base64_type &last_block_id, block_height_type last_height, timestamp_t last_time, const base64_type &last_hash,
                    const base64_type &prev_block_id) {
  return unresolved_block_pool->setPool(last_block_id, last_height, last_time, last_hash, prev_block_id);
}

} // namespace gruut
