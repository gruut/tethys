#include "include/chain.hpp"
#include "../../../lib/appbase/include/application.hpp"
#include "../../../lib/tethys-utils/src/ags.hpp"
#include <regex>

namespace tethys {

class ChainImpl {
public:
  ChainImpl(Chain &self) : self(self) {}

  void initWorld(nlohmann::json &world_state) {
    world_type world = unmarshalWorldState(world_state);

    appbase::app().setWorldId(world.world_id);

    string auth_cert;
    int multiline_size = world.authority_cert.size() - 1;
    for (int i = 0; i <= multiline_size; ++i) {
      auth_cert += world.authority_cert[i];
      if (i != multiline_size)
        auth_cert += "\n";
    }

    appbase::app().setAuthCert(auth_cert);

    self.saveWorld(world);
    self.saveLatestWorldId(world.world_id);
  }

  optional<vector<string>> initChain(nlohmann::json &chain_state) {
    local_chain_type chain = unmarshalChainState(chain_state);

    if (appbase::app().getWorldId() != chain.world_id)
      return {};

    appbase::app().setChainId(chain.chain_id);

    self.saveChain(chain);
    self.saveLatestChainId(chain.chain_id);

    return chain.tracker_addresses;
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
      logger::ERROR("Failed to parse world json: {}", e.what());
      throw e;
    }
  }

  local_chain_type unmarshalChainState(nlohmann::json &state) {
    try {
      local_chain_type chain_state;

      chain_state.chain_id = state["/chain/id"_json_pointer];
      chain_state.world_id = state["/chain/world"_json_pointer];
      chain_state.chain_created_time = state["/chain/after"_json_pointer];

      chain_state.allow_custom_contract = state["/policy/allow_custom_contract"_json_pointer];
      chain_state.allow_oracle = state["/policy/allow_oracle"_json_pointer];
      chain_state.allow_tag = state["/policy/allow_tag"_json_pointer];
      chain_state.allow_heavy_contract = state["/policy/allow_heavy_contract"_json_pointer];

      chain_state.creator_id = state["/creator/id"_json_pointer];
      chain_state.creator_cert = state["creator"]["cert"].get<vector<string>>();
      chain_state.creator_sig = state["/creator/sig"_json_pointer];

      for (auto &tracker_info : state["tracker"]) {
        auto address = tracker_info["address"].get<string>();
        chain_state.tracker_addresses.emplace_back(address);
      }

      return chain_state;
    } catch (nlohmann::json::parse_error &e) {
      logger::ERROR("Failed to parse local chain json: {}", e.what());
      throw e;
    }
  }

  Chain &self;
};

void Chain::initWorld(nlohmann::json &world_state) {
  impl->initWorld(world_state);
}

optional<vector<string>> Chain::initChain(nlohmann::json &chain_state) {
  return impl->initChain(chain_state);
}

Chain::Chain(string_view dbms, string_view table_name, string_view db_user_id, string_view db_password) {
  impl = make_unique<ChainImpl>(*this);
  rdb_controller = make_unique<RdbController>(dbms, table_name, db_user_id, db_password);
  kv_controller = make_unique<KvController>();
  unresolved_block_pool = make_unique<UnresolvedBlockPool>();
  m_us_tree = StateTree();
  m_cs_tree = StateTree();
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
  const string condition = "ORDER BY block_height DESC LIMIT 1";
  auto block = rdb_controller->getBlock(condition);

  if (block.has_value()) {
    return block.value().getHeight();
  } else {
    return 0;
  }
}

// KV functions
void Chain::saveLatestWorldId(const alphanumeric_type &world_id) {
  kv_controller->saveLatestWorldId(world_id);
}

void Chain::saveLatestChainId(const alphanumeric_type &chain_id) {
  kv_controller->saveLatestChainId(chain_id);
}

void Chain::saveWorld(world_type &world_info) {
  kv_controller->saveWorld(world_info);
}

void Chain::saveChain(local_chain_type &chain_info) {
  kv_controller->saveChain(chain_info);
}

void Chain::saveBlockIds() {
  nlohmann::json id_array = unresolved_block_pool->getPoolBlockIds();

  kv_controller->saveBlockIds(TypeConverter::bytesToString(nlohmann::json::to_cbor(id_array)));
}

void Chain::saveBackupBlock(const nlohmann::json &block_json) {
  base58_type block_id = json::get<string>(block_json["block"], "id").value();
  string serialized_block = TypeConverter::toString(nlohmann::json::to_cbor(block_json));

  kv_controller->saveBackupBlock(block_id, serialized_block);
}

void Chain::saveBackupResult(const UnresolvedBlock &UR_block) {
  base58_type block_id = UR_block.block.getBlockId();
  kv_controller->saveBackupUserLedgers(block_id, unresolved_block_pool->serializeUserLedgerList(UR_block));
  kv_controller->saveBackupContractLedgers(block_id, unresolved_block_pool->serializeContractLedgerList(UR_block));
  kv_controller->saveBackupUserAttributes(block_id, unresolved_block_pool->serializeUserAttributeList(UR_block));
  kv_controller->saveBackupUserCerts(block_id, unresolved_block_pool->serializeUserCertList(UR_block));
  kv_controller->saveBackupContracts(block_id, unresolved_block_pool->serializeContractList(UR_block));
}

void Chain::saveSelfInfo(self_info_type &self_info) {
  kv_controller->saveSelfInfo(self_info);
}

string Chain::getValueByKey(string what, const string &base_keys) {
  return kv_controller->getValueByKey(what, base_keys);
}

void Chain::restorePool() {
  // TODO: 예외처리
  string serialized_id_array = kv_controller->loadBlockIds();
  nlohmann::json id_array_json = nlohmann::json::from_cbor(serialized_id_array);
  for (auto &each_block_id : id_array_json) {
    base58_type block_id = each_block_id;

    // TODO: serialized_id_array가 empty string인 경우, from_cbor에서 exception 처리
    // TODO: initialize 가 실패하는 case 처리
    nlohmann::json block_msg = nlohmann::json::from_cbor(kv_controller->loadBackupBlock(block_id));
    Block restored_block;
    restored_block.initialize(block_msg);
    block_push_result_type push_result = unresolved_block_pool->pushBlock(restored_block);

    UnresolvedBlock restored_unresolved_block = unresolved_block_pool->findBlock(restored_block.getBlockId(), push_result.height);

    nlohmann::json user_ledger_list_json = nlohmann::json::from_cbor(kv_controller->loadBackupUserLedgers(block_id));
    nlohmann::json contract_ledger_list_json = nlohmann::json::from_cbor(kv_controller->loadBackupContractLedgers(block_id));
    nlohmann::json user_attribute_list_json = nlohmann::json::from_cbor(kv_controller->loadBackupUserAttributes(block_id));
    nlohmann::json user_cert_list_json = nlohmann::json::from_cbor(kv_controller->loadBackupUserCerts(block_id));
    nlohmann::json contract_list_json = nlohmann::json::from_cbor(kv_controller->loadBackupContracts(block_id));

    restoreUserLedgerList(restored_unresolved_block, user_ledger_list_json);
    restoreContractLedgerList(restored_unresolved_block, contract_ledger_list_json);
    restoreUserAttributeList(restored_unresolved_block, user_attribute_list_json);
    restoreUserCertList(restored_unresolved_block, user_cert_list_json);
    restoreContractList(restored_unresolved_block, contract_list_json);
  }
}

void Chain::restoreUserLedgerList(UnresolvedBlock &restored_unresolved_block, const nlohmann::json &user_ledger_list_json) {
  for (auto &each_user_ledger_json : user_ledger_list_json) {
    user_ledger_type each_user_ledger;

    each_user_ledger.var_name = json::get<string>(each_user_ledger_json, "var_name").value();
    each_user_ledger.var_val = json::get<string>(each_user_ledger_json, "var_val").value();
    each_user_ledger.var_type = stoi(json::get<string>(each_user_ledger_json, "var_type").value());
    each_user_ledger.uid = json::get<string>(each_user_ledger_json, "uid").value();
    each_user_ledger.up_time = static_cast<tethys::timestamp_t>(stoll(json::get<string>(each_user_ledger_json, "up_time").value()));
    each_user_ledger.up_block = stoi(json::get<string>(each_user_ledger_json, "up_block").value());
    each_user_ledger.tag = json::get<string>(each_user_ledger_json, "tag").value();
    each_user_ledger.pid = json::get<string>(each_user_ledger_json, "pid").value();
    each_user_ledger.query_type = static_cast<tethys::QueryType>(stoi(json::get<string>(each_user_ledger_json, "query_type").value()));
    each_user_ledger.is_empty = json::get<bool>(each_user_ledger_json, "is_empty").value();

    restored_unresolved_block.user_ledger_list[each_user_ledger.pid] = each_user_ledger;
  }
}

void Chain::restoreContractLedgerList(UnresolvedBlock &restored_unresolved_block, const nlohmann::json &contract_ledger_list_json) {
  for (auto &each_contract_ledger_json : contract_ledger_list_json) {
    contract_ledger_type each_contract_ledger;

    each_contract_ledger.var_name = json::get<string>(each_contract_ledger_json, "var_name").value();
    each_contract_ledger.var_val = json::get<string>(each_contract_ledger_json, "var_val").value();
    each_contract_ledger.var_type = stoi(json::get<string>(each_contract_ledger_json, "var_type").value());
    each_contract_ledger.cid = json::get<string>(each_contract_ledger_json, "cid").value();
    each_contract_ledger.up_time = static_cast<tethys::timestamp_t>(stoll(json::get<string>(each_contract_ledger_json, "up_time").value()));
    each_contract_ledger.up_block = stoi(json::get<string>(each_contract_ledger_json, "up_block").value());
    each_contract_ledger.var_info = json::get<string>(each_contract_ledger_json, "var_info").value();
    each_contract_ledger.pid = json::get<string>(each_contract_ledger_json, "pid").value();
    each_contract_ledger.query_type =
        static_cast<tethys::QueryType>(stoi(json::get<string>(each_contract_ledger_json, "query_type").value()));
    each_contract_ledger.is_empty = json::get<bool>(each_contract_ledger_json, "is_empty").value();

    restored_unresolved_block.contract_ledger_list[each_contract_ledger.pid] = each_contract_ledger;
  }
}

void Chain::restoreUserAttributeList(UnresolvedBlock &restored_unresolved_block, const nlohmann::json &user_attribute_list_json) {
  for (auto &each_user_attribute_json : user_attribute_list_json) {
    user_attribute_type each_user_attribute;

    each_user_attribute.uid = json::get<string>(each_user_attribute_json, "uid").value();
    each_user_attribute.register_day =
        static_cast<tethys::timestamp_t>(stoll(json::get<string>(each_user_attribute_json, "register_day").value()));
    each_user_attribute.register_code = json::get<string>(each_user_attribute_json, "register_code").value();
    each_user_attribute.gender = stoi(json::get<string>(each_user_attribute_json, "gender").value());
    each_user_attribute.isc_type = json::get<string>(each_user_attribute_json, "isc_type").value();
    each_user_attribute.isc_code = json::get<string>(each_user_attribute_json, "isc_code").value();
    each_user_attribute.location = json::get<string>(each_user_attribute_json, "location").value();
    each_user_attribute.age_limit = stoi(json::get<string>(each_user_attribute_json, "age_limit").value());
    each_user_attribute.sigma = json::get<string>(each_user_attribute_json, "sigma").value();

    restored_unresolved_block.user_attribute_list[each_user_attribute.uid] = each_user_attribute;
  }
}

void Chain::restoreUserCertList(UnresolvedBlock &restored_unresolved_block, const nlohmann::json &user_cert_list_json) {
  for (auto &each_user_cert_json : user_cert_list_json) {
    user_cert_type each_user_cert;

    each_user_cert.uid = json::get<string>(each_user_cert_json, "uid").value();
    each_user_cert.sn = json::get<string>(each_user_cert_json, "sn").value();
    each_user_cert.nvbefore = static_cast<tethys::timestamp_t>(stoll(json::get<string>(each_user_cert_json, "nvbefore").value()));
    each_user_cert.nvafter = static_cast<tethys::timestamp_t>(stoll(json::get<string>(each_user_cert_json, "nvafter").value()));
    each_user_cert.x509 = json::get<string>(each_user_cert_json, "x509").value();

    restored_unresolved_block.user_cert_list[each_user_cert.sn] = each_user_cert;
  }
}

void Chain::restoreContractList(UnresolvedBlock &restored_unresolved_block, const nlohmann::json &contract_list_json) {
  for (auto &each_contract_json : contract_list_json) {
    contract_type each_contract;

    each_contract.cid = json::get<string>(each_contract_json, "cid").value();
    each_contract.after = static_cast<tethys::timestamp_t>(stoll(json::get<string>(each_contract_json, "after").value()));
    each_contract.before = static_cast<tethys::timestamp_t>(stoll(json::get<string>(each_contract_json, "before").value()));
    each_contract.author = json::get<string>(each_contract_json, "author").value();
    each_contract.friends = json::get<string>(each_contract_json, "friends").value();
    each_contract.contract = json::get<string>(each_contract_json, "contract").value();
    each_contract.desc = json::get<string>(each_contract_json, "desc").value();
    each_contract.sigma = json::get<string>(each_contract_json, "sigma").value();

    restored_unresolved_block.contract_list[each_contract.cid] = each_contract;
  }
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
  // TODO: user cert에는 한 사람이 유효기간이 서로 다른 인증서 여러 개가 저장될 수 있음
  user_cert_type user_cert;

  user_cert.uid = result_info.user;
  user_cert.sn = json::get<string>(option, "sn").value();
  user_cert.nvbefore = static_cast<uint64_t>(stoll(json::get<string>(option, "notbefore").value()));
  user_cert.nvafter = static_cast<uint64_t>(stoll(json::get<string>(option, "notafter").value()));
  user_cert.x509 = json::get<string>(option, "x509").value();

  UR_block.user_cert_list[user_cert.sn] = user_cert;

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

  contract_info.contract = json::get<string>(option, "contract").value();
  contract_info.desc = json::get<string>(option, "desc").value();
  contract_info.sigma = json::get<string>(option, "sigma").value();

  UR_block.contract_list[contract_info.cid] = contract_info;

  return true;
}

bool Chain::queryContractDisable(UnresolvedBlock &UR_block, nlohmann::json &option, result_query_info_type &result_info) {
  // TODO: $user = cid.author인 경우에만 허용
  contract_type contract_info;

  contract_info.cid = json::get<string>(option, "cid").value();
  contract_info.before = TimeUtil::nowBigInt();

  // TODO: before 값을 갱신하는 코드
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
  string pid = user_ledger.pid;

  user_ledger_type found = findUserLedgerFromHead(pid);

  if (!found.is_empty) {
    logger::ERROR("Same pid state is exist. v.create must be new.");
    return false;
  }

  user_ledger.var_val = amount;
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

  auto pid = json::get<string>(option, "pid");
  string calculated_pid;

  string tag = json::get<string>(option, "tag").value(); // TODO: var_info도 가능한 것 아닌가?
  int var_type;

  // Transfer : from
  if (from == "contract") {
    // from : contract
    string cid = result_info.self;
    contract_ledger_type contract_ledger;

    var_type = getVarType(cid, unit, UR_block.block.getHeight(), UR_block.cur_vec_idx);

    calculated_pid = calculatePid(pid, unit, var_type, from, UR_block.block.getHeight(), UR_block.cur_vec_idx);

    contract_ledger_type found = findContractLedgerFromHead(calculated_pid);

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

    UR_block.contract_ledger_list[calculated_pid] = contract_ledger;
  } else {
    // from : user
    base58_type uid;
    if (from == "user")
      uid = result_info.user;
    else if (from == "author")
      uid = result_info.author;

    var_type = getVarType(uid, unit, UR_block.block.getHeight(), UR_block.cur_vec_idx);

    user_ledger_type user_ledger(unit, var_type, uid, tag);

    calculated_pid = calculatePid(pid, unit, var_type, uid, UR_block.block.getHeight(), UR_block.cur_vec_idx);

    user_ledger_type found = findUserLedgerFromHead(calculated_pid);

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

    UR_block.user_ledger_list[calculated_pid] = user_ledger;
  }

  // Transfer : to
  if (isContractId(to)) {
    // to : contract
    contract_ledger_type contract_ledger;
    contract_ledger_type found_ledger = findContractLedgerFromHead(calculated_pid);

    // TODO: 이 로직은 수정 필요.
    if (found_ledger.var_val == "")
      contract_ledger.query_type = QueryType::INSERT;
    else
      contract_ledger.query_type = QueryType::UPDATE;

    int modified_value = stoi(found_ledger.var_val) + stoi(amount);
    contract_ledger.var_val = to_string(modified_value);
    contract_ledger.var_info = from;

    UR_block.contract_ledger_list[calculated_pid] = contract_ledger;
  } else if (isUserId(to)) {
    // to : user
    user_ledger_type user_ledger;
    user_ledger_type found_ledger = findUserLedgerFromHead(calculated_pid);
    if (found_ledger.uid != result_info.user)
      return false;

    // TODO: 이 로직은 수정 필요.
    if (found_ledger.var_val == "")
      user_ledger.query_type = QueryType::INSERT;
    else
      user_ledger.query_type = QueryType::UPDATE;

    int modified_value = stoi(found_ledger.var_val) + stoi(amount);
    user_ledger.var_val = to_string(modified_value);
    user_ledger.tag = tag;

    UR_block.user_ledger_list[calculated_pid] = user_ledger;
  }

  return true;
}

bool Chain::queryUserScope(UnresolvedBlock &UR_block, nlohmann::json &option, result_query_info_type &result_info) {
  // TODO: authority 검사
  string var_name;
  base58_type uid;
  string var_val;
  string calculated_pid;
  string tag;
  int var_type;
  QueryType query_type;

  var_name = json::get<string>(option, "name").value();

  string target = json::get<string>(option, "for").value(); // user or author
  if (target == "user") {
    uid = result_info.user;
  } else if (target == "author") {
    uid = result_info.author;
  } else {
    logger::ERROR("target is not 'user' or 'author' at `queryUserScope`");
  }

  var_val = json::get<string>(option, "value").value();

  auto type_json = json::get<string>(option, "type");
  if (type_json.has_value()) { // type 존재: 새 변수 생성
    query_type = QueryType::INSERT;
    var_type = stoi(type_json.value());
  } else { // type 생략: 기존 변수 수정
    query_type = QueryType::UPDATE;
    var_type = getVarType(uid, var_name, UR_block.block.getHeight(), UR_block.cur_vec_idx);

    if (var_type == (int)UniqueCheck::NOT_UNIQUE)
      return false;
  }

  auto pid_json = json::get<string>(option, "pid");
  auto tag_json = json::get<string>(option, "tag");
  if (tag_json.has_value()) {
    tag = tag_json.value();
    calculated_pid = calculatePid(pid_json, var_name, var_type, uid, tag);
  } else
    calculated_pid = calculatePid(pid_json, var_name, var_type, uid, UR_block.block.getHeight(), UR_block.cur_vec_idx);

  // TODO: '통화'속성의 변수는 변경 불가능한 조건 검사 시행

  user_ledger_type user_ledger;

  user_ledger.var_name = var_name;
  user_ledger.var_val = var_val;
  user_ledger.var_type = var_type;
  user_ledger.uid = uid;
  user_ledger.up_time = TimeUtil::nowBigInt(); // TODO: DB에 저장되는 시간인지, mem_ledger에 들어오는 시간인지
  user_ledger.up_block = UR_block.block.getHeight();
  user_ledger.tag = tag; // TODO: 변수가 존재하는 경우 무시
  user_ledger.pid = calculated_pid;
  user_ledger.query_type = query_type;

  UR_block.user_ledger_list[calculated_pid] = user_ledger;

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

  calculatePid(pid, var_name, var_type, cid, UR_block.block.getHeight(), UR_block.cur_vec_idx);

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

string Chain::calculatePid(optional<string> &pid, string &var_name, int var_type, string &var_owner, const block_height_type height,
                           const int vec_idx) {
  string calculated_pid;
  if (pid.has_value()) {
    calculated_pid = pid.value();
  } else {
    BytesBuilder bytes_builder;
    bytes_builder.append(var_name);
    bytes_builder.appendDec(var_type);
    if (isContractId(var_owner))
      bytes_builder.append(var_owner);
    else if (isUserId(var_owner))
      bytes_builder.appendBase<58>(var_owner);
    calculated_pid = TypeConverter::bytesToString(Sha256::hash(bytes_builder.getBytes()));

    if (!checkUniqueVarName(var_owner, var_name, height, vec_idx)) {
      logger::ERROR("Error in `calculatePid`, there was several same (var owner, var name).");
      calculated_pid.clear();
    }
  }
  return calculated_pid;
}

string Chain::calculatePid(optional<string> &pid, string &var_name, int var_type, string &var_owner, string &tag_varInfo) {
  string calculated_pid;
  if (pid.has_value()) {
    calculated_pid = pid.value();
  } else {
    BytesBuilder bytes_builder;
    bytes_builder.append(var_name);
    bytes_builder.appendDec(var_type);
    if (isContractId(var_owner))
      bytes_builder.append(var_owner);
    else if (isUserId(var_owner))
      bytes_builder.appendBase<58>(var_owner);
    bytes_builder.append(tag_varInfo);
    calculated_pid = TypeConverter::bytesToString(Sha256::hash(bytes_builder.getBytes()));
  }
  return calculated_pid;
}

int Chain::getVarType(const string &var_owner, const string &var_name, const block_height_type height, const int vec_idx) {
  int var_type = (int)UniqueCheck::NO_VALUE;

  int pool_deque_idx = height - unresolved_block_pool->getLatestConfirmedHeight() - 1;
  int pool_vec_idx = vec_idx;

  if (isUserId(var_owner)) {
    map<string, user_ledger_type> current_user_ledgers;

    // unresolved block pool의 연결된 앞 블록들에서 검색
    while (pool_deque_idx >= 0) {
      current_user_ledgers = unresolved_block_pool->getBlock(pool_deque_idx, pool_vec_idx).user_ledger_list;
      for (auto &each_ledger : current_user_ledgers) {
        if ((each_ledger.second.uid == var_owner) && (each_ledger.second.var_name == var_name)) {
          if ((var_type != (int)UniqueCheck::NO_VALUE) && (var_type != each_ledger.second.var_type)) {
            return (int)UniqueCheck::NOT_UNIQUE; // var_type가 초기값이 아닌데 새로운 값이 오면 unique하지 않다는 것
          }
          var_type = each_ledger.second.var_type;
        }
      }
      pool_vec_idx = unresolved_block_pool->getBlock(pool_deque_idx, pool_vec_idx).prev_vec_idx;
      --pool_deque_idx;
    }
  } else if (isContractId(var_owner)) {
    map<string, contract_ledger_type> current_contract_ledgers;

    // unresolved block pool의 연결된 앞 블록들에서 검색
    while (pool_deque_idx >= 0) {
      current_contract_ledgers = unresolved_block_pool->getBlock(pool_deque_idx, pool_vec_idx).contract_ledger_list;
      for (auto &each_ledger : current_contract_ledgers) {
        if ((each_ledger.second.cid == var_owner) && (each_ledger.second.var_name == var_name)) {
          if ((var_type != (int)UniqueCheck::NO_VALUE) && (var_type != each_ledger.second.var_type)) {
            return (int)UniqueCheck::NOT_UNIQUE; // var_type가 초기값이 아닌데 새로운 값이 오면 unique하지 않다는 것
          }
          var_type = each_ledger.second.var_type;
        }
      }
      pool_vec_idx = unresolved_block_pool->getBlock(pool_deque_idx, pool_vec_idx).prev_vec_idx;
      --pool_deque_idx;
    }
  }

  // rdb에서 검색
  int rdb_var_type = rdb_controller->getVarTypeFromRDB(var_owner, var_name);

  if (rdb_var_type == (int)UniqueCheck::NOT_UNIQUE)
    return (int)UniqueCheck::NOT_UNIQUE;

  if ((var_type == (int)UniqueCheck::NO_VALUE) && (rdb_var_type == (int)UniqueCheck::NO_VALUE)) {
    return (int)UniqueCheck::NO_VALUE;
  } else if ((var_type >= 0) && (rdb_var_type >= 0)) { // pool에서도, rdb에서도 값이 하나씩 발견
    if (var_type == rdb_var_type)
      return var_type;
    else
      return (int)UniqueCheck::NOT_UNIQUE;
  } else { // pool과 rdb에서 하나는 NO_VALUE, 하나는 하나의 값
    if (var_type == (int)UniqueCheck::NO_VALUE)
      return rdb_var_type;
    else if (rdb_var_type == (int)UniqueCheck::NO_VALUE)
      return var_type;
  }

  logger::ERROR("Something error in `getVarType`. Cannot reach here.");
}

bool Chain::checkUniqueVarName(const string &var_owner, const string &var_name, const block_height_type height, const int vec_idx) {
  int result = getVarType(var_owner, var_name, height, vec_idx);
  if (result == (int)UniqueCheck::NOT_UNIQUE)
    return false;
  else
    return true;
}

block_push_result_type Chain::pushBlock(Block &block) {
  return unresolved_block_pool->pushBlock(block);
}

UnresolvedBlock Chain::findBlock(const base58_type &block_id, const block_height_type block_height) {
  return unresolved_block_pool->findBlock(block_id, block_height);
}

bool Chain::resolveBlock(Block &block, UnresolvedBlock &resolved_result) {
  vector<base58_type> dropped_block_ids;
  if (!unresolved_block_pool->resolveBlock(block, resolved_result, dropped_block_ids))
    return false;

  for (auto &each_block_id : dropped_block_ids) {
    kv_controller->delBackup(each_block_id);
  }

  return true;
}

void Chain::setPool(const base64_type &last_block_id, block_height_type last_height, timestamp_t last_time, const base64_type &last_hash,
                    const base64_type &prev_block_id) {
  return unresolved_block_pool->setPool(last_block_id, last_height, last_time, last_hash, prev_block_id);
}

search_result_type Chain::findUserLedgerFromPoint(const string &pid, block_height_type height, int vec_idx) {
  search_result_type search_result;
  int pool_deque_idx = height - unresolved_block_pool->getLatestConfirmedHeight() - 1;
  int pool_vec_idx = vec_idx;

  map<string, user_ledger_type> current_user_ledgers;
  map<string, user_ledger_type>::iterator it;

  while (1) {
    current_user_ledgers = unresolved_block_pool->getBlock(pool_deque_idx, pool_vec_idx).user_ledger_list;

    it = current_user_ledgers.find(pid);
    if (it != current_user_ledgers.end()) {
      search_result.user_ledger = it->second;
      return search_result;
    }
    pool_vec_idx = unresolved_block_pool->getBlock(pool_deque_idx, pool_vec_idx).prev_vec_idx;
    --pool_deque_idx;

    if (pool_deque_idx < 0) {
      // -1이 되면 rdb에서 찾을 차례. select문으로 조회하는데도 찾지 못한다면 존재하지 않는 데이터
      bool result = rdb_controller->findUserScopeFromRDB(pid, search_result.user_ledger);
      if (!result) {
        search_result.not_found = true;
      }
      return search_result;
    }
  }
}

search_result_type Chain::findContractLedgerFromPoint(const string &pid, block_height_type height, int vec_idx) {
  search_result_type search_result;
  int pool_deque_idx = height - unresolved_block_pool->getLatestConfirmedHeight() - 1;
  int pool_vec_idx = vec_idx;

  map<string, contract_ledger_type> current_contract_ledgers;
  map<string, contract_ledger_type>::iterator it;

  while (1) {
    current_contract_ledgers = unresolved_block_pool->getBlock(pool_deque_idx, pool_vec_idx).contract_ledger_list;

    it = current_contract_ledgers.find(pid);
    if (it != current_contract_ledgers.end()) {
      search_result.contract_ledger = it->second;
      return search_result;
    }
    pool_vec_idx = unresolved_block_pool->getBlock(pool_deque_idx, pool_vec_idx).prev_vec_idx;
    --pool_deque_idx;

    if (pool_deque_idx < 0) {
      // -1이 되면 rdb에서 찾을 차례. select문으로 조회하는데도 찾지 못한다면 존재하지 않는 데이터
      bool result = rdb_controller->findContractScopeFromRDB(pid, search_result.contract_ledger);
      if (!result) {
        search_result.not_found = true;
      }
      return search_result;
    }
  }
}

bool Chain::isUserId(const string &id) {
  const auto BASE58_REGEX = "^[A-HJ-NP-Za-km-z1-9]*$";
  regex rgx(BASE58_REGEX);
  if (id.size() != static_cast<int>(44) || !regex_match(id, rgx)) {
    logger::ERROR("Invalid user ID.");
    return false;
  }
  return true;
}

bool Chain::isContractId(const string &id) {
  // TODO: 검증 추가
  if (id.size() > 45)
    return true;
  else
    return false;
}

// State Tree
void Chain::setupStateTree() {
  m_us_tree.updateUserState(rdb_controller->getAllUserLedger());
  m_cs_tree.updateContractState(rdb_controller->getAllContractLedger());
}

void Chain::updateStateTree(const UnresolvedBlock &unresolved_block) {
  m_us_tree.updateUserState(unresolved_block.user_ledger_list);
  m_cs_tree.updateContractState(unresolved_block.contract_ledger_list);
  // TODO: user cert나 contract 등도 update해야 함
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

user_ledger_type Chain::findUserLedgerFromHead(const string &pid) {
  if (m_us_tree.getMerkleNode(pid) == nullptr) {
    user_ledger_type empty_ledger;
    empty_ledger.is_empty = true;
    return empty_ledger;
  } else
    return m_us_tree.getMerkleNode(pid)->getUserLedger();
}

contract_ledger_type Chain::findContractLedgerFromHead(const string &pid) {
  if (m_cs_tree.getMerkleNode(pid) == nullptr) {
    contract_ledger_type empty_ledger;
    empty_ledger.is_empty = true;
    return empty_ledger;
  } else
    return m_cs_tree.getMerkleNode(pid)->getContractLedger();
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
