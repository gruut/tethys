#include "include/unresolved_block_pool.hpp"

namespace tethys {

UnresolvedBlockPool::UnresolvedBlockPool() {
  logger::INFO("Unresolved block pool is created");
  m_block_pool.clear();
}

void UnresolvedBlockPool::setPool(const base64_type &last_block_id, block_height_type last_height, timestamp_t last_time,
                                  const base64_type &last_hash, const base64_type &prev_block_id) {
  m_latest_confirmed_id = last_block_id;
  m_latest_confirmed_height = last_height;
  m_latest_confirmed_time = last_time;
  m_latest_confirmed_hash = last_hash;
  m_latest_confirmed_prev_id = prev_block_id;
}

inline int32_t UnresolvedBlockPool::size() {
  return m_block_pool.size();
}

inline bool UnresolvedBlockPool::empty() {
  return m_block_pool.empty();
}

inline void UnresolvedBlockPool::clear() {
  m_block_pool.clear();
}

base58_type UnresolvedBlockPool::getLatestConfirmedId() {
  return m_latest_confirmed_id;
}

block_height_type UnresolvedBlockPool::getLatestConfirmedHeight() {
  return m_latest_confirmed_height;
}

timestamp_t UnresolvedBlockPool::getLatestConfiremdTime() {
  return m_latest_confirmed_time;
}

base64_type UnresolvedBlockPool::getLatestConfiremdHash() {
  return m_latest_confirmed_hash;
}

base58_type UnresolvedBlockPool::getLatestConfiremdPrevId() {
  return m_latest_confirmed_prev_id;
}

// 블록이 push될 때마다 실행되는 함수. pool이 되는 deque를 resize 하는 등의 컨트롤을 한다.
bool UnresolvedBlockPool::prepareDeque(block_height_type t_height) {
  std::lock_guard<std::recursive_mutex> guard(m_push_mutex);
  if (m_latest_confirmed_height >= t_height) {
    return false;
  }

  if ((TimeUtil::nowBigInt() - m_latest_confirmed_time) < (t_height - m_latest_confirmed_height - 1) * config::BP_INTERVAL) {
    return false;
  }

  int deq_idx = static_cast<int>(t_height - m_latest_confirmed_height) - 1; // deque에서 위치하는 인덱스. e.g., 0 = 2 - 1 - 1
  if (m_block_pool.size() < deq_idx + 1) {
    m_block_pool.resize(deq_idx + 1);
  }

  return true;
}

block_push_result_type UnresolvedBlockPool::pushBlock(Block &new_block) {
  logger::INFO("Unresolved block pool: pushBlock called");
  block_push_result_type ret_val;
  ret_val.height = 0;
  ret_val.linked = false;
  ret_val.duplicated = false;

  std::lock_guard<std::recursive_mutex> guard(m_push_mutex);

  block_height_type block_height = new_block.getHeight();

  int deq_idx = static_cast<int>(block_height - m_latest_confirmed_height) - 1; // e.g., 0 = 2 - 1 - 1
  if (!prepareDeque(block_height))
    return ret_val;

  for (auto &each_block : m_block_pool[deq_idx]) {
    if (each_block.block == new_block) {
      ret_val.height = block_height;
      ret_val.duplicated = true;
      return ret_val;
    }
  }

  int prev_vec_idx = -1; // no previous

  if (deq_idx > 0) { // if there is previous bin
    int idx = 0;
    for (auto &each_block : m_block_pool[deq_idx - 1]) {
      if (each_block.block.getBlockId() == new_block.getPrevBlockId()) {
        prev_vec_idx = static_cast<int>(idx);
        break;
      }
      ++idx;
    }
  } else { // no previous
    if (new_block.getPrevBlockId() == m_latest_confirmed_id) {
      prev_vec_idx = 0;
    } else {
      logger::ERROR("drop block -- this is not linkable block!");
      return ret_val;
    }
  }

  int vec_idx = m_block_pool[deq_idx].size();

  m_block_pool[deq_idx].emplace_back(new_block, vec_idx, prev_vec_idx); // pool에 블록 추가
  ret_val.height = block_height;

  if (deq_idx + 1 < m_block_pool.size()) { // if there is next bin
    for (auto &each_block : m_block_pool[deq_idx + 1]) {
      if (each_block.block.getPrevBlockId() == new_block.getBlockId()) {
        if (each_block.prev_vec_idx < 0) {
          each_block.prev_vec_idx = vec_idx;
        }
      }
    }
  }

  return ret_val;
}

bool UnresolvedBlockPool::resolveBlock(Block &new_block, UnresolvedBlock &resolved_result, vector<base58_type> &dropped_block_id) {
  dropped_block_id.clear();

  //  if (!lateStage(new_block)) {
  //    return false;
  //  }

  if (new_block.getHeight() - m_latest_confirmed_height > config::BLOCK_CONFIRM_LEVEL) {
    updateTotalNumSSig();

    if (m_block_pool.size() < 2 || m_block_pool[0].empty() || m_block_pool[1].empty()) {
      return false;
    }

    int highest_total_ssig = 0;
    int resolved_block_idx = -1;

    for (int i = 0; i < m_block_pool[0].size(); ++i) {
      if (m_block_pool[0][i].prev_vec_idx == 0 && m_block_pool[0][i].ssig_sum > highest_total_ssig) {
        highest_total_ssig = m_block_pool[0][i].ssig_sum;
        resolved_block_idx = i;
      }
    }

    if (resolved_block_idx < 0 || highest_total_ssig < config::MIN_SIGNATURE_COLLECT_SIZE) {
      return false;
    }

    bool is_after = false;
    for (auto &each_block : m_block_pool[1]) {
      if (each_block.prev_vec_idx == resolved_block_idx) { // some block links this block
        is_after = true;
        break;
      }
    }

    if (!is_after) {
      return false;
    }

    m_latest_confirmed_prev_id = m_latest_confirmed_id;

    m_latest_confirmed_id = m_block_pool[0][resolved_block_idx].block.getBlockId();
    m_latest_confirmed_hash = m_block_pool[0][resolved_block_idx].block.getBlockHash();
    m_latest_confirmed_height = m_block_pool[0][resolved_block_idx].block.getHeight();

    UnresolvedBlock resolved_block = m_block_pool[0][resolved_block_idx];
    for (auto &each_block : m_block_pool[0]) {
      dropped_block_id.push_back(each_block.block.getBlockId());
    }

    // TODO: pop되기 전에 반복문을 사용하면 선택받지 못한 block branch를 전부 지울 수 있을 것이나,
    //  그 후에 만약 해당 블록에 연결된 블록이 들어와서 지웠던 블록을 다시 요청하는 경우가 생길 수 있는것은 아닐지 고려.

    m_block_pool.pop_front();

    if (m_block_pool.empty()) {
      resolved_result = resolved_block;
      return true;
    }

    for (auto &each_block : m_block_pool[0]) {
      if (each_block.block.getPrevBlockId() == m_latest_confirmed_id) {
        each_block.prev_vec_idx = 0;
      } else {
        // this block is unlinkable => to be deleted
        each_block.prev_vec_idx = -1;
      }
    }
    resolved_result = resolved_block;
    return true;
  } else {
    return false;
  }
}

void UnresolvedBlockPool::updateTotalNumSSig() {
  if (m_block_pool.empty())
    return;

  for (auto &each_level : m_block_pool) {
    for (auto &each_block : each_level) {
      each_block.ssig_sum = each_block.block.getNumSigners();
    }
  }

  for (int i = (int)m_block_pool.size() - 1; i > 0; --i) {
    for (auto &each_block : m_block_pool[i]) { // for vector
      if (each_block.prev_vec_idx >= 0 && m_block_pool[i - 1].size() > each_block.prev_vec_idx) {
        m_block_pool[i - 1][each_block.prev_vec_idx].ssig_sum += each_block.ssig_sum;
      }
    }
  }
}

UnresolvedBlock UnresolvedBlockPool::getUnresolvedBlock(const base58_type &block_id, const block_height_type block_height) {
  // TODO: Refactoring. height와 cur_vec_idx를 이용하면 순회 안 하고도 찾을 수 있음
  for (auto &each_level : m_block_pool) {
    for (auto &each_block : each_level) {
      if ((each_block.block.getBlockId() == block_id) && (each_block.block.getHeight() == block_height))
        return each_block;
    }
  }

  logger::ERROR("Cannot found block in pool " + block_id + " " + to_string(block_height));
  return UnresolvedBlock{};
}

UnresolvedBlock UnresolvedBlockPool::getUnresolvedBlock(int pool_deq_idx, int pool_vec_idx) {
  return m_block_pool[pool_deq_idx][pool_vec_idx];
}

void UnresolvedBlockPool::setUnresolvedBlock(const UnresolvedBlock &unresolved_block) {
  int pool_deq_idx = static_cast<int>(unresolved_block.block.getHeight() - m_latest_confirmed_height) - 1;
  int pool_vec_idx = unresolved_block.cur_vec_idx;

  m_block_pool[pool_deq_idx][pool_vec_idx] = unresolved_block;
  m_block_pool[pool_deq_idx][pool_vec_idx].is_processed = true;
}

vector<int> UnresolvedBlockPool::getPath(const base58_type &block_id, const block_height_type block_height) {
  vector<int> path;
  if (!block_id.empty()) {
    // latest_confirmed의 height가 10이었고, 현재 line을 구하려는 블록의 height가 12라면 vector size는 2이다
    int vec_size = static_cast<int>(block_height - m_latest_confirmed_height);
    path.resize(vec_size, -1);

    int current_deq_idx = static_cast<int>(block_height - m_latest_confirmed_height) - 1;
    int current_vec_idx = -1;

    for (int i = 0; i < m_block_pool[current_deq_idx].size(); ++i) {
      if (block_id == m_block_pool[current_deq_idx][i].block.getBlockId()) {
        current_vec_idx = static_cast<int>(i);
        break;
      }
    }

    path[current_deq_idx] = current_vec_idx;

    while (current_deq_idx > 0) {
      current_vec_idx = m_block_pool[current_deq_idx][current_vec_idx].prev_vec_idx;
      current_deq_idx--;

      path[current_deq_idx] = current_vec_idx;
    }

    if (m_block_pool[current_deq_idx][current_vec_idx].prev_vec_idx != 0) {
      logger::ERROR("This block is not linked!");
      path.resize(1, -1);
    }
    return path;
  }
}

Block &UnresolvedBlockPool::getLowestUnprocessedBlock(const block_pool_info_type &longest_chain_info) {
  int current_deq_idx = longest_chain_info.deq_idx;
  int current_vec_idx = longest_chain_info.vec_idx;

  while (current_deq_idx >= 0) {
    int prev_vec_idx = m_block_pool[current_deq_idx][current_vec_idx].prev_vec_idx;

    bool current_processed = m_block_pool[current_deq_idx][current_vec_idx].is_processed;
    bool prev_processed = m_block_pool[current_deq_idx - 1][prev_vec_idx].is_processed;

    if ((current_processed == false) && (prev_processed == true)) {
      return m_block_pool[current_deq_idx][current_vec_idx].block;
    }

    current_vec_idx = prev_vec_idx;
    --current_deq_idx;
  }
}

nlohmann::json UnresolvedBlockPool::getPoolBlockIds() {
  nlohmann::json id_array = nlohmann::json::array();

  for (auto &each_level : m_block_pool) {
    for (auto &each_block : each_level) {
      std::string block_id = each_block.block.getBlockId();
      id_array.push_back(block_id);
    }
  }
  return id_array;
}

string UnresolvedBlockPool::serializeUserLedgerList(const UnresolvedBlock &unresolved_block) {
  nlohmann::json ledger_list_json = nlohmann::json::array();
  string serialized_list;

  for (auto &each_ledger : unresolved_block.user_ledger_list) {
    nlohmann::json json;
    json["var_name"] = each_ledger.second.var_name;
    json["var_value"] = each_ledger.second.var_value;
    json["var_type"] = to_string(each_ledger.second.var_type);
    json["uid"] = each_ledger.second.uid;
    json["up_time"] = to_string(each_ledger.second.up_time);
    json["up_block"] = to_string(each_ledger.second.up_block);
    json["tag"] = each_ledger.second.tag;
    json["pid"] = each_ledger.second.pid;
    json["query_type"] = each_ledger.second.query_type;
    json["is_empty"] = each_ledger.second.is_empty;

    ledger_list_json.push_back(json);
  }

  serialized_list = TypeConverter::bytesToString(nlohmann::json::to_cbor(ledger_list_json));
  return serialized_list;
}

string UnresolvedBlockPool::serializeContractLedgerList(const UnresolvedBlock &unresolved_block) {
  nlohmann::json ledger_list_json = nlohmann::json::array();
  string serialized_list;

  for (auto &each_ledger : unresolved_block.contract_ledger_list) {
    nlohmann::json json;
    json["var_name"] = each_ledger.second.var_name;
    json["var_value"] = each_ledger.second.var_value;
    json["var_type"] = to_string(each_ledger.second.var_type);
    json["cid"] = each_ledger.second.cid;
    json["up_time"] = to_string(each_ledger.second.up_time);
    json["up_block"] = to_string(each_ledger.second.up_block);
    json["var_info"] = each_ledger.second.var_info;
    json["pid"] = each_ledger.second.pid;
    json["query_type"] = each_ledger.second.query_type;
    json["is_empty"] = each_ledger.second.is_empty;

    ledger_list_json.push_back(json);
  }

  serialized_list = TypeConverter::bytesToString(nlohmann::json::to_cbor(ledger_list_json));
  return serialized_list;
}

string UnresolvedBlockPool::serializeUserAttributeList(const UnresolvedBlock &unresolved_block) {
  nlohmann::json ledger_list_json = nlohmann::json::array();
  string serialized_list;

  for (auto &each_user : unresolved_block.user_attribute_list) {
    nlohmann::json json;
    json["uid"] = each_user.second.uid;
    json["register_day"] = to_string(each_user.second.register_day);
    json["register_code"] = each_user.second.register_code;
    json["gender"] = to_string(each_user.second.gender);
    json["isc_type"] = each_user.second.isc_type;
    json["isc_code"] = each_user.second.isc_code;
    json["location"] = each_user.second.location;
    json["age_limit"] = to_string(each_user.second.age_limit);
    json["sigma"] = each_user.second.sigma;

    ledger_list_json.push_back(json);
  }

  serialized_list = TypeConverter::bytesToString(nlohmann::json::to_cbor(ledger_list_json));
  return serialized_list;
}

string UnresolvedBlockPool::serializeUserCertList(const UnresolvedBlock &unresolved_block) {
  nlohmann::json ledger_list_json = nlohmann::json::array();
  string serialized_list;

  for (auto &each_cert : unresolved_block.user_cert_list) {
    nlohmann::json json;
    json["uid"] = each_cert.second.uid;
    json["sn"] = each_cert.second.sn;
    json["nvbefore"] = to_string(each_cert.second.nvbefore);
    json["nvafter"] = to_string(each_cert.second.nvafter);
    json["x509"] = each_cert.second.x509;

    ledger_list_json.push_back(json);
  }

  serialized_list = TypeConverter::bytesToString(nlohmann::json::to_cbor(ledger_list_json));
  return serialized_list;
}

string UnresolvedBlockPool::serializeContractList(const UnresolvedBlock &unresolved_block) {
  nlohmann::json ledger_list_json = nlohmann::json::array();
  string serialized_list;

  for (auto &each_contract : unresolved_block.contract_list) {
    nlohmann::json json;
    json["cid"] = each_contract.second.cid;
    json["after"] = to_string(each_contract.second.after);
    json["before"] = to_string(each_contract.second.before);
    json["author"] = each_contract.second.author;
    json["friends"] = each_contract.second.friends;
    json["contract"] = each_contract.second.contract;
    json["desc"] = each_contract.second.desc;
    json["sigma"] = each_contract.second.sigma;

    ledger_list_json.push_back(json);
  }

  serialized_list = TypeConverter::bytesToString(nlohmann::json::to_cbor(ledger_list_json));
  return serialized_list;
}

} // namespace tethys
