#include "include/chain.hpp"
#include "../../../lib/appbase/include/application.hpp"
#include "../../../lib/tethys-utils/src/ags.hpp"

namespace tethys {

class ChainImpl {
public:
  ChainImpl(Chain &self) : self(self) {}

  void initWorld(nlohmann::json &world_state) {
    world_type world = unmarshalWorldState(world_state);

    appbase::app().setWorldId(world.world_id);

    self.saveWorld(world);
  }

  world_type unmarshalWorldState(nlohmann::json &state) {
    try {
      world_type world_state;

      world_state.world_id = state["/world/id"_json_pointer];
      world_state.world_created_time = state["/world/after"_json_pointer];

      world_state.keyc_name = state["/key_currency/name"_json_pointer];
      world_state.initial_amount = state["/key_currency/initial_amount"_json_pointer];

      world_state.allow_mining = state["/mining_policy/allow_mining"_json_pointer];

      world_state.allow_anonymous_user = state["/user_policy/allow_anonymous_user"_json_pointer];
      world_state.join_fee = state["/user_policy/join_fee"_json_pointer];

      world_state.eden_sig = state["eden_sig"].get<string>();

      world_state.authority_id = state["/authority/id"_json_pointer];
      world_state.authority_cert = state["authority"]["cert"].get<vector<string>>();

      world_state.creator_id = state["/creator/id"_json_pointer];
      world_state.creator_cert = state["creator"]["cert"].get<vector<string>>();
      world_state.creator_sig = state["/creator/sig"_json_pointer];

      return world_state;
    } catch (nlohmann::json::parse_error &e) {
      logger::ERROR("[LOAD WORLD] Failed to parse world_create.json: {}", e.what());
      throw e;
    }
  }

  Chain &self;
};

void Chain::initWorld(nlohmann::json &world_state) {
  impl->initWorld(world_state);
}

Chain::Chain(string_view dbms, string_view table_name, string_view db_user_id, string_view db_password) {
  impl = make_unique<ChainImpl>(*this);
  rdb_controller = make_unique<RdbController>(dbms, table_name, db_user_id, db_password);
  kv_controller = make_unique<KvController>();
  unresolved_block_pool = make_unique<UnresolvedBlockPool>();
}

Chain::~Chain() {
  impl.reset();
}

// RDB functions
string Chain::getUserCert(const base58_type &user_id) {
  return rdb_controller->getUserCert(user_id);
}

bool Chain::applyBlockToRDB(const tethys::Block &block_info) {
  return rdb_controller->applyBlockToRDB(block_info);
}

bool Chain::applyTransactionToRDB(const tethys::Block &block_info) {
  return rdb_controller->applyTransactionToRDB(block_info);
}

bool Chain::applyUserLedgerToRDB(const std::map<string, user_ledger_type> &user_ledger_list) {
  return rdb_controller->applyUserLedgerToRDB(user_ledger_list);
}

bool Chain::applyContractLedgerToRDB(const std::map<string, contract_ledger_type> &contract_ledger_list) {
  return rdb_controller->applyContractLedgerToRDB(contract_ledger_list);
}

bool Chain::applyUserAttributeToRDB(const std::map<base58_type, user_attribute_type> &user_attribute_list) {
  return rdb_controller->applyUserAttributeToRDB(user_attribute_list);
}

bool Chain::applyUserCertToRDB(const std::map<base58_type, user_cert_type> &user_cert_list) {
  return rdb_controller->applyUserCertToRDB(user_cert_list);
}

bool Chain::applyContractToRDB(const std::map<base58_type, contract_type> &contract_list) {
  return rdb_controller->applyContractToRDB(contract_list);
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

bool Chain::findUserFromRDB(string key, user_ledger_type &user_ledger) {
  return rdb_controller->findUserFromRDB(key, user_ledger);
}

bool Chain::findContractFromRDB(string key, contract_ledger_type &contract_ledger) {
  return rdb_controller->findContractFromRDB(key, contract_ledger);
}

int Chain::getVarType(string &key) {
  return rdb_controller->getVarType(key);
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

string Chain::getValueByKey(string what, const string &base_keys) {
  return kv_controller->getValueByKey(what, base_keys);
}

// Unresolved block pool functions
bool Chain::queryUserJoin(UnresolvedBlock &UR_block, nlohmann::json &option, result_query_info_type &result_info) {
  user_attribute_type user_info;

  user_info.uid = result_info.user;
  user_info.register_day = static_cast<tethys::timestamp_t>(stoll(json::get<string>(option, "register_day").value()));
  user_info.register_code = json::get<string>(option, "register_code").value();
  user_info.gender = stoi(json::get<string>(option, "gender").value());
  user_info.isc_type = json::get<string>(option, "isc_type").value();
  user_info.isc_code = json::get<string>(option, "isc_code").value();
  user_info.location = json::get<string>(option, "location").value();
  user_info.age_limit = stoi(json::get<string>(option, "age_limit").value());

  string msg = user_info.uid + to_string(user_info.register_day) + user_info.register_code + to_string(user_info.gender) +
               user_info.isc_type + user_info.isc_code + user_info.location + to_string(user_info.age_limit);

  // TODO: signature by tethys Authority 정확히 구현
  AGS ags;
  /// auto sigma = ags.sign(TethysAuthority_secret_key, msg);
  user_info.sigma = "TODO: signature by Tethys Authority";

  UR_block.user_attribute_list[user_info.uid] = user_info;

  return true;
}

bool Chain::queryUserCert(UnresolvedBlock &UR_block, nlohmann::json &option, result_query_info_type &result_info) {
  user_cert_type user_cert;

  user_cert.uid = result_info.user;
  user_cert.sn = json::get<string>(option, "sn").value();
  user_cert.nvbefore = static_cast<uint64_t>(stoll(json::get<string>(option, "notbefore").value()));
  user_cert.nvafter = static_cast<uint64_t>(stoll(json::get<string>(option, "notafter").value()));
  user_cert.x509 = json::get<string>(option, "x509").value();

  UR_block.user_cert_list[user_cert.uid] = user_cert;

  return true;
}

bool Chain::queryContractNew(UnresolvedBlock &UR_block, nlohmann::json &option, result_query_info_type &result_info) {
  contract_type contract_info;

  contract_info.cid = json::get<string>(option, "cid").value();
  contract_info.after = static_cast<uint64_t>(stoll(json::get<string>(option, "after").value()));
  contract_info.before = static_cast<uint64_t>(stoll(json::get<string>(option, "before").value()));
  contract_info.author = json::get<string>(option, "author").value();
  // TODO: friend 추가할 때 자신을 friend로 추가한 contract를 찾아서 추가해야 함
  contract_id_type friends = json::get<string>(option, "friend").value();
  string contract = json::get<string>(option, "contract").value();
  string desc = json::get<string>(option, "desc").value();
  string sigma = json::get<string>(option, "sigma").value();

  UR_block.contract_list[contract_info.cid] = contract_info;

  return true;
}

bool Chain::queryContractDisable(UnresolvedBlock &UR_block, nlohmann::json &option, result_query_info_type &result_info) {
  // TODO: $user = cid.author인 경우에만 허용
  contract_id_type cid = json::get<string>(option, "cid").value();
  timestamp_t before = TimeUtil::nowBigInt();

  return true;
}

bool Chain::queryIncinerate(UnresolvedBlock &UR_block, nlohmann::json &option, result_query_info_type &result_info) {
  string amount = json::get<string>(option, "amount").value();
  string pid = json::get<string>(option, "pid").value();

  // TODO: pointer로 인한 복사/접근 체크 다시 할 것. 복사되어야 함.
  user_ledger_type found = findUserLedgerFromHead(pid);

  if (found.is_empty)
    return false;

  if (found.uid != result_info.user)
    return false;
  int modified_value = stoi(found.var_val) - stoi(amount);
  found.var_val = to_string(modified_value);

  if (modified_value == 0)
    found.query_type = QueryType::DELETE;
  else if (modified_value > 0)
    found.query_type = QueryType::UPDATE;
  else
    logger::ERROR("var_val is under 0 in queryIncinerate!");

  UR_block.user_ledger_list[pid] = found;

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

  user_ledger_type found = findUserLedgerFromHead(pid);

  if (found.is_empty) {
    found.var_val = "";
  } else {
    if (found.uid != result_info.user)
      return false;
  }

  int modified_value = stoi(found.var_val) + stoi(amount);
  user_ledger.var_val = to_string(modified_value);

  user_ledger.up_time = up_time;
  user_ledger.up_block = up_block;
  user_ledger.query_type = QueryType::INSERT;

  UR_block.user_ledger_list[pid] = user_ledger;

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
  int var_type;

  // Transfer : from
  if (from == "contract") {
    // from : contract
    string cid = result_info.self;
    contract_ledger_type contract_ledger;

    var_type = getVarType(cid);

    key = pidCheck(pid, unit, var_type, from);
    if (pid.has_value()) {
      contract_ledger.pid = TypeConverter::stringToBytes(pid.value());
    }

    contract_ledger_type found = findContractLedgerFromHead(key);

    if (found.is_empty) {
      logger::ERROR("Not exist from's ledger");
      return false;
    }

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

    UR_block.contract_ledger_list[key] = contract_ledger;
  } else {
    // from : user
    base58_type uid;
    if (from == "user")
      uid = result_info.user;
    else if (from == "author")
      uid = result_info.author;

    var_type = getVarType(uid);

    user_ledger_type user_ledger(unit, var_type, uid, tag);

    key = pidCheck(pid, unit, var_type, uid);
    if (pid.has_value()) {
      user_ledger.pid = TypeConverter::stringToBytes(pid.value());
    }

    user_ledger_type found = findUserLedgerFromHead(key);

    if (found.is_empty) {
      logger::ERROR("Not exist from's ledger");
      return false;
    }

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

    UR_block.user_ledger_list[key] = user_ledger;
  }

  // Transfer : to
  if (to.size() > 45) {
    // to : contract
    contract_ledger_type contract_ledger;

    key = pidCheck(pid, unit, var_type, to);
    if (pid.has_value()) {
      contract_ledger.pid = TypeConverter::stringToBytes(pid.value());
    }

    contract_ledger_type found = findContractLedgerFromHead(key);

    if (found.var_val == "")
      contract_ledger.query_type = QueryType::INSERT;
    else
      contract_ledger.query_type = QueryType::UPDATE;

    int modified_value = stoi(found.var_val) + stoi(amount);
    contract_ledger.var_val = to_string(modified_value);
    contract_ledger.var_info = from;

    UR_block.contract_ledger_list[key] = contract_ledger;
  } else {
    // to : user
    user_ledger_type user_ledger;

    key = pidCheck(pid, unit, var_type, to);
    if (pid.has_value()) {
      user_ledger.pid = TypeConverter::stringToBytes(pid.value());
    }

    user_ledger_type found = findUserLedgerFromHead(key);
    if (found.uid != result_info.user)
      return false;

    if (found.var_val == "")
      user_ledger.query_type = QueryType::INSERT;
    else
      user_ledger.query_type = QueryType::UPDATE;

    int modified_value = stoi(found.var_val) + stoi(amount);
    user_ledger.var_val = to_string(modified_value);

    user_ledger.tag = tag;

    UR_block.user_ledger_list[key] = user_ledger;
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

  UR_block.user_ledger_list[key] = user_ledger;

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

  UR_block.contract_ledger_list[key] = contract_ledger;

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
    BytesBuilder bytes_builder;
    bytes_builder.append(var_name);
    bytes_builder.append(var_type);
    bytes_builder.append(var_owner);
    key = TypeConverter::bytesToString(Sha256::hash(bytes_builder.getBytes()));

    rdb_controller->checkUnique();

    // TODO: unique 확인을 위해 어떤 방법으로 체크할 것인지. key 일괄 통일과 관련되어있음.
  }

  return key;
}

search_result_type Chain::findUserLedgerFromPoint(string key, block_height_type height, int vec_idx) {
  search_result_type search_result;
  int pool_deque_idx = height - unresolved_block_pool->getLatestConfirmedHeight() - 1;
  int pool_vec_idx = vec_idx;

  map<string, user_ledger_type> current_user_ledgers = unresolved_block_pool->getBlock(pool_deque_idx, pool_vec_idx).user_ledger_list;
  map<string, user_ledger_type>::iterator it;

  while (1) {
    it = current_user_ledgers.find(key);
    if (it != current_user_ledgers.end()) {
      search_result.user_ledger = it->second;
      return search_result;
    }
    pool_vec_idx = unresolved_block_pool->getBlock(pool_deque_idx, pool_vec_idx).prev_vec_idx;
    --pool_deque_idx;

    if (pool_deque_idx < 0) {
      // -1이 되면 rdb에서 찾을 차례. select문으로 조회하는데도 찾지 못한다면 존재하지 않는 데이터
      bool result = findUserFromRDB(key, search_result.user_ledger);
      if (!result) {
        search_result.not_found = true;
      }
      return search_result;
    }
  }
}

search_result_type Chain::findContractLedgerFromPoint(string key, block_height_type height, int vec_idx) {
  search_result_type search_result;
  int pool_deque_idx = height - unresolved_block_pool->getLatestConfirmedHeight() - 1;
  int pool_vec_idx = vec_idx;

  map<string, contract_ledger_type> current_contract_ledgers =
      unresolved_block_pool->getBlock(pool_deque_idx, pool_vec_idx).contract_ledger_list;
  map<string, contract_ledger_type>::iterator it;

  while (1) {
    it = current_contract_ledgers.find(key);
    if (it != current_contract_ledgers.end()) {
      search_result.contract_ledger = it->second;
      return search_result;
    }
    pool_vec_idx = unresolved_block_pool->getBlock(pool_deque_idx, pool_vec_idx).prev_vec_idx;
    --pool_deque_idx;

    if (pool_deque_idx < 0) {
      // -1이 되면 rdb에서 찾을 차례. select문으로 조회하는데도 찾지 못한다면 존재하지 않는 데이터
      bool result = findContractFromRDB(key, search_result.contract_ledger);
      if (!result) {
        search_result.not_found = true;
      }
      return search_result;
    }
  }
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

void Chain::setPool(const base64_type &last_block_id, block_height_type last_height, timestamp_t last_time, const base64_type &last_hash,
                    const base64_type &prev_block_id) {
  return unresolved_block_pool->setPool(last_block_id, last_height, last_time, last_hash, prev_block_id);
}

void Chain::setupStateTree() {
  m_us_tree.setupTree(rdb_controller->getAllUserLedger());
  m_cs_tree.setupTree(rdb_controller->getAllContractLedger());
}

void Chain::updateStateTree(const UnresolvedBlock &unresolved_block) {
  m_us_tree.updateState(unresolved_block.user_ledger_list);
  m_cs_tree.updateState(unresolved_block.contract_ledger_list);
  // TODO: user cert나 contract 관련 사항들은 state tree에 반영할 필요는 없고, 그냥 ledger로써만 가지고 있으면 되나?
  //  아니면 user_cert는 보류한다고 해도 contract는 직접적으로 연관되어 있으므로 반영을 해야 하는가?
}

void Chain::revertStateTree(const UnresolvedBlock &unresolved_block) {
  for (auto &each_user_ledger : unresolved_block.user_ledger_list) {
    m_us_tree.insertNode(
        findUserLedgerFromPoint(each_user_ledger.first, unresolved_block.block.getHeight(), unresolved_block.cur_vec_idx).user_ledger);
  }

  for (auto &each_contract_ledger : unresolved_block.contract_ledger_list) {
    m_us_tree.insertNode(
        findContractLedgerFromPoint(each_contract_ledger.first, unresolved_block.block.getHeight(), unresolved_block.cur_vec_idx)
            .contract_ledger);
  }
  // TODO: user cert나 contract 등도 revert해야함
}

user_ledger_type Chain::findUserLedgerFromHead(string key) {
  if (m_us_tree.getMerkleNode(key) == nullptr) {
    user_ledger_type empty_ledger;
    empty_ledger.is_empty = true;
    return empty_ledger;
  } else
    return m_us_tree.getMerkleNode(key)->getUserLedger();
}

contract_ledger_type Chain::findContractLedgerFromHead(string key) {
  if (m_cs_tree.getMerkleNode(key) == nullptr) {
    contract_ledger_type empty_ledger;
    empty_ledger.is_empty = true;
    return empty_ledger;
  } else
    return m_cs_tree.getMerkleNode(key)->getContractLedger();
}

void Chain::moveHead(const base58_type &target_block_id, const block_height_type target_block_height) {
  if (!target_block_id.empty()) {
    // latest_confirmed의 height가 10이었고, 현재 head의 height가 11이라면 m_block_pool[0]에 있어야 한다
    int current_deq_idx = m_head_deq_idx;
    int current_vec_idx = m_head_vec_idx;
    int current_height = static_cast<int>(m_head_height);

    while (current_height > target_block_height) {
      current_vec_idx = unresolved_block_pool->getBlock(current_deq_idx, current_vec_idx).prev_vec_idx;
      --current_deq_idx;
      --current_height;
    }

    vector<int> line = unresolved_block_pool->getLine(target_block_id, target_block_height);

    while (current_vec_idx != line[current_deq_idx]) {
      current_vec_idx = unresolved_block_pool->getBlock(current_deq_idx, current_vec_idx).prev_vec_idx;
      --current_deq_idx;
      --current_height;
    }

    int common_deq_idx = current_deq_idx;
    int common_vec_idx = current_vec_idx;
    int common_height = static_cast<int>(current_height);
    int front_count = static_cast<int>(target_block_height) - common_height;
    int back_count = static_cast<int>(m_head_height) - common_height;

    if (common_deq_idx < 0) {
      logger::ERROR("URBP, Something error in move_head() - Cannot find pool element");
      return;
    }

    current_height = static_cast<int>(m_head_height);
    current_deq_idx = m_head_deq_idx;
    current_vec_idx = m_head_vec_idx;

    for (int i = 0; i < back_count; i++) {
      revertStateTree(unresolved_block_pool->getBlock(current_deq_idx, current_vec_idx));

      current_vec_idx = unresolved_block_pool->getBlock(current_deq_idx, current_vec_idx).prev_vec_idx;
      current_deq_idx--;
      current_height--;
    }

    for (int i = 0; i < front_count; i++) {
      updateStateTree(unresolved_block_pool->getBlock(current_deq_idx, current_vec_idx));

      current_vec_idx = line[current_deq_idx];
      ++current_deq_idx;
      ++current_height;
    }

    m_head_id = unresolved_block_pool->getBlock(m_head_deq_idx, m_head_vec_idx).block.getBlockId();
    m_head_height = current_height;
    m_head_deq_idx = current_deq_idx;
    m_head_vec_idx = current_vec_idx;

    if (unresolved_block_pool->getBlock(m_head_deq_idx, m_head_vec_idx).block.getHeight() != m_head_height) {
      logger::ERROR("URBP, Something error in move_head() - end part, check height");
      return;
    }

    // TODO: missing link 등에 대한 예외처리를 추가해야 할 필요 있음
  }
}

base58_type Chain::getCurrentHeadId() {
  return m_head_id;
}

block_height_type Chain::getCurrentHeadHeight() {
  return m_head_height;
}

bytes Chain::getUserStateRoot() {
  return m_us_tree.getRootValue();
}

bytes Chain::getContractStateRoot() {
  return m_cs_tree.getRootValue();
}

} // namespace tethys
