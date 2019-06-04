#include "include/unresolved_block_pool.hpp"

namespace gruut {

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

// 블록이 push될 때마다 실행되는 함수. pool이 되는 벡터를 resize 하는 등의 컨트롤을 한다.
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

ubp_push_result_type UnresolvedBlockPool::pushBlock(Block &block, bool is_restore) {
  logger::INFO("Unresolved block pool: pushBlock called");
  ubp_push_result_type ret_val; // 해당 return 구조는 추후 변경 가능성 있음
  ret_val.height = 0;
  ret_val.linked = false;
  ret_val.duplicated = false;

  std::lock_guard<std::recursive_mutex> guard(m_push_mutex);

  block_height_type block_height = block.getHeight();

  int deq_idx = static_cast<int>(block_height - m_latest_confirmed_height) - 1; // e.g., 0 = 2 - 1 - 1
  if (!prepareDeque(block_height))
    return ret_val;

  for (auto &each_block : m_block_pool[deq_idx]) {
    if (each_block.block == block) {
      ret_val.height = block_height;
      ret_val.duplicated = true;
      return ret_val;
    }
  }

  int prev_vec_idx = -1; // no previous

  if (deq_idx > 0) { // if there is previous bin
    int idx = 0;
    for (auto &each_block : m_block_pool[deq_idx - 1]) {
      if (each_block.block.getBlockId() == block.getPrevBlockId()) {
        prev_vec_idx = static_cast<int>(idx);
        break;
      }
      ++idx;
    }
  } else { // no previous
    if (block.getPrevBlockId() == m_latest_confirmed_id) {
      prev_vec_idx = 0;
    } else {
      logger::ERROR("drop block -- this is not linkable block!");
      return ret_val;
    }
  }

  int vec_idx = m_block_pool[deq_idx].size();

  m_block_pool[deq_idx].emplace_back(block, prev_vec_idx); // pool에 블록 추가

  ret_val.height = block_height;

  if (!is_restore)
    backupPool();

  if (deq_idx + 1 < m_block_pool.size()) { // if there is next bin
    for (auto &each_block : m_block_pool[deq_idx + 1]) {
      if (each_block.block.getPrevBlockId() == block.getBlockId()) {
        if (each_block.prev_vector_idx < 0) {
          each_block.prev_vector_idx = vec_idx;
        }
      }
    }
  }

  invalidateCaches(); // 캐시 관련 함수. 추후 고려.

  return ret_val;
}

UnresolvedBlock UnresolvedBlockPool::findBlock(const base58_type &block_id, const block_height_type block_height) {
  for (auto &each_level : m_block_pool) {
    for (auto &each_block : each_level) {
      if ((each_block.block.getBlockId() == block_id) && (each_block.block.getHeight() == block_height))
        return each_block;
    }
  }

  logger::ERROR("Cannot found block in pool " + block_id + " " + to_string(block_height));
  return UnresolvedBlock{};
}

UnresolvedBlock UnresolvedBlockPool::getBlock(int pool_deq_idx, int pool_vec_idx) {
  return m_block_pool[pool_deq_idx][pool_vec_idx];
}

vector<int> UnresolvedBlockPool::getLine(const base58_type &block_id, const block_height_type block_height) {
  vector<int> line;
  if (!block_id.empty()) {
    // latest_confirmed의 height가 10이었고, 현재 line을 구하려는 블록의 height가 12라면 vector size는 2이다
    int vec_size = static_cast<int>(block_height - m_latest_confirmed_height);
    line.resize(vec_size, -1);

    int current_deq_idx = static_cast<int>(block_height - m_latest_confirmed_height) - 1;
    int current_vec_idx = -1;

    for (int i = 0; i < m_block_pool[current_deq_idx].size(); ++i) {
      if (block_id == m_block_pool[current_deq_idx][i].block.getBlockId()) {
        current_vec_idx = static_cast<int>(i);
        break;
      }
    }

    line[current_deq_idx] = current_vec_idx;

    while (current_deq_idx > 0) {
      current_vec_idx = m_block_pool[current_deq_idx][current_vec_idx].prev_vector_idx;
      current_deq_idx--;

      line[current_deq_idx] = current_vec_idx;
    }

    if (m_block_pool[current_deq_idx][current_vec_idx].prev_vector_idx != 0) {
      logger::ERROR("This block is not linked!");
      line.resize(1, -1);
    }
    return line;
  }
}

bool UnresolvedBlockPool::resolveBlock(Block &block, UnresolvedBlock &resolved_result) {
  //  if (!lateStage(block)) {
  //    return false;
  //  }

  if (block.getHeight() - m_latest_confirmed_height > config::BLOCK_CONFIRM_LEVEL) {
    updateTotalNumSSig();

    if (m_block_pool.size() < 2 || m_block_pool[0].empty() || m_block_pool[1].empty()) {
      return false;
    }

    int highest_total_ssig = 0;
    int resolved_block_idx = -1;

    for (int i = 0; i < m_block_pool[0].size(); ++i) {
      if (m_block_pool[0][i].prev_vector_idx == 0 && m_block_pool[0][i].ssig_sum > highest_total_ssig) {
        highest_total_ssig = m_block_pool[0][i].ssig_sum;
        resolved_block_idx = i;
      }
    }

    if (resolved_block_idx < 0 || highest_total_ssig < config::MIN_SIGNATURE_COLLECT_SIZE) {
      return false;
    }

    bool is_after = false;
    for (auto &each_block : m_block_pool[1]) {
      if (each_block.prev_vector_idx == resolved_block_idx) { // some block links this block
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

    // TODO: pop되기 전에 반복문을 사용하면 선택받지 못한 block branch를 전부 지울 수 있을 것.
    //  그런데 그 후에 만약 해당 블록에 연결된 블록이 들어와서 지웠던 블록을 다시 요청하는 경우가 생길 수 있는것은 아닐지 고려.

    m_block_pool.pop_front();

    if (m_block_pool.empty()) {
      resolved_result = resolved_block;
      return true;
    }

    for (auto &each_block : m_block_pool[0]) {
      if (each_block.block.getPrevBlockId() == m_latest_confirmed_id) {
        each_block.prev_vector_idx = 0;
      } else {
        // this block is unlinkable => to be deleted
        each_block.prev_vector_idx = -1;
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
      if (each_block.prev_vector_idx >= 0 && m_block_pool[i - 1].size() > each_block.prev_vector_idx) {
        m_block_pool[i - 1][each_block.prev_vector_idx].ssig_sum += each_block.ssig_sum;
      }
    }
  }
}

// ------------------------------------------------------------------
//
// void KvController::testBackward() {
//  cout << "--------------- test Backward called -----------------" << endl;
//  Layer back_layer = popBackLayer();
//
//  for (auto data : back_layer.m_temporary_data) {
//    Key key = data.first;
//    Value value = data.second;
//    uint32_t path;
//    test_data rollback_data;
//
//    rollback_data.user_id = key.user_id;
//    rollback_data.var_type = key.var_type;
//    rollback_data.var_name = key.var_name;
//
//    cout << key << ", " << value << endl;
//
//    // 지워진 데이터라면 addNode 를 호출하여 다시 트리에 삽입
//    if (value.isDeleted) {
//      rollback_data.var_value = value.var_value;
//      path = value.path;
//
//      m_tree.addNode(path, rollback_data);
//    } else {
//      // 지워진 데이터가 아니라면 남은 레이어와 DB 에서 찾아봄
//      int depth = checkLayer(key);
//      // 데이터가 존재한다면 modifyNode 호출하여 이전값으로 돌림
//      if (depth != NO_DATA) {
//        if (depth == DB_DATA) {
//          cout << "data is in the DB" << endl;
//          pair<int, vector<string>> db_data = m_server.selectAllUsingUserIdVarTypeVarName(key.user_id, key.var_type, key.var_name);
//
//          rollback_data.var_value = db_data.second[VAR_VALUE];
//          path = (uint32_t)stoul(db_data.second[PATH]);
//        } else {
//          cout << "data is in the m_layer[" << depth << "]" << endl;
//          map<Key, Value>::iterator it;
//          it = m_layer[depth].m_temporary_data.find(key);
//
//          rollback_data.var_value = it->second.var_value;
//          path = it->second.path;
//        }
//        m_tree.modifyNode(path, rollback_data);
//      }
//      // 데이터가 존재하지 않는다면 removeNode 호출하여 트리에서 제거
//      else {
//        cout << "can't find data" << endl;
//
//        path = value.path;
//        m_tree.removeNode(path);
//      }
//    }
//  }
//}

// 추후 구현
void UnresolvedBlockPool::invalidateCaches() {}
void UnresolvedBlockPool::backupPool() {}
nlohmann::json UnresolvedBlockPool::readBackupIds() {}

} // namespace gruut
