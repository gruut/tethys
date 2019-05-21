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

// 블록이 push될 때마다 실행되는 함수. pool이 되는 벡터를 resize 하는 등의 컨트롤을 한다.
bool UnresolvedBlockPool::prepareBins(block_height_type t_height) {
  std::lock_guard<std::recursive_mutex> guard(m_push_mutex);
  if (m_latest_confirmed_height >= t_height) {
    return false;
  }

  if ((TimeUtil::nowBigInt() - m_latest_confirmed_time) < (t_height - m_latest_confirmed_height - 1) * config::BP_INTERVAL) {
    return false;
  }

  int bin_pos = static_cast<int>(t_height - m_latest_confirmed_height) - 1; // deque에서 위치하는 인덱스. e.g., 0 = 2 - 1 - 1
  if (m_block_pool.size() < bin_pos + 1) {
    m_block_pool.resize(bin_pos + 1);
  }

  return true;
}

ubp_push_result_type UnresolvedBlockPool::push(Block &block, bool is_restore) {
  logger::INFO("Unresolved block pool: push called");
  ubp_push_result_type ret_val; // 해당 return 구조는 추후 변경 가능성 있음
  ret_val.height = 0;
  ret_val.linked = false;
  ret_val.duplicated = false;

  std::lock_guard<std::recursive_mutex> guard(m_push_mutex);

  block_height_type block_height = block.getHeight();

  int bin_idx = static_cast<int>(block_height - m_latest_confirmed_height) - 1; // e.g., 0 = 2 - 1 - 1
  if (!prepareBins(block_height))
    return ret_val;

  for (auto &each_block : m_block_pool[bin_idx]) {
    if (each_block.block == block) {
      ret_val.height = block_height;
      ret_val.duplicated = true;
      return ret_val;
    }
  }

  int prev_queue_idx = -1; // no previous

  if (bin_idx > 0) { // if there is previous bin
    int idx = 0;
    for (auto &each_block : m_block_pool[bin_idx - 1]) {
      if (each_block.block.getBlockId() == block.getPrevBlockId()) {
        prev_queue_idx = static_cast<int>(idx);
        break;
      }
      ++idx;
    }
  } else { // no previous
    if (block.getPrevBlockId() == m_latest_confirmed_id) {
      prev_queue_idx = 0;
    } else {
      logger::ERROR("drop block -- this is not linkable block!");
      return ret_val;
    }
  }

  int queue_idx = m_block_pool[bin_idx].size();

  m_block_pool[bin_idx].emplace_back(block, prev_queue_idx); // pool에 블록 추가

  ret_val.height = block_height;

  if (!is_restore)
    backupPool();

  if (bin_idx + 1 < m_block_pool.size()) { // if there is next bin
    for (auto &each_block : m_block_pool[bin_idx + 1]) {
      if (each_block.block.getPrevBlockId() == block.getBlockId()) {
        if (each_block.prev_vector_idx < 0) {
          each_block.prev_vector_idx = queue_idx;
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

  logger::ERROR("Cannot found block" + block_id + " " + to_string(block_height));
  return UnresolvedBlock{};
}

void UnresolvedBlockPool::moveHead(const base64_type &target_block_id_b64, const block_height_type target_block_height) {
  if (!target_block_id_b64.empty()) {
    // latest_confirmed의 height가 10이었고, 현재 옮기려는 타겟의 height가 12라면 m_block_pool[1]에 있어야 한다
    int target_height = target_block_height;
    int target_bin_idx = static_cast<int>(target_block_height - m_latest_confirmed_height) - 1;
    int target_queue_idx = -1;

    // new_head를 pool에서 어디에 있는지 찾음
    for (int i = 0; i < m_block_pool[target_bin_idx].size(); ++i) {
      if (target_block_id_b64 == m_block_pool[target_bin_idx][i].block.getBlockId()) {
        target_queue_idx = static_cast<int>(i);
        break;
      }
    }

    // 현재 head부터 confirmed까지의 경로 구함 -> vector를 resize해서 depth에 맞춰서 집어넣음
    // latest_confirmed의 height가 10이었고, 현재 head의 height가 11이라면 m_block_pool[0]에 있어야 한다
    int current_height = static_cast<int>(m_head_height);
    int current_bin_idx = m_head_bin_idx;
    int current_queue_idx = m_head_queue_idx;

    if (target_queue_idx == -1 || current_queue_idx == -1) {
      logger::ERROR("URBP, Something error in move_head() - Cannot find pool element");
      return;
    }

    // height가 높은 쪽을 아래로 가지고 내려오고, height가 같아지면 서로 하나씩 내려가며 비교
    int back_count = 0;
    int front_count = 0;
    std::deque<pair<int, int>> where_to_go;
    where_to_go.clear();

    while (target_height != current_height) {
      if (target_height > current_height) {
        where_to_go.emplace_front(target_bin_idx, target_queue_idx);
        target_queue_idx = m_block_pool[target_bin_idx][target_queue_idx].prev_vector_idx;
        target_height--;
        target_bin_idx--;

        front_count++;
      } else if (target_height < current_height) {
        current_queue_idx = m_block_pool[current_bin_idx][current_queue_idx].prev_vector_idx;
        current_height--;
        current_bin_idx--;

        back_count++;
      }
    }

    while ((target_bin_idx != current_bin_idx) || (target_queue_idx != current_queue_idx)) {
      where_to_go.emplace_front(target_bin_idx, target_queue_idx);
      target_queue_idx = m_block_pool[target_bin_idx][target_queue_idx].prev_vector_idx;
      target_height--;
      target_bin_idx--;
      front_count++;

      current_queue_idx = m_block_pool[current_bin_idx][current_queue_idx].prev_vector_idx;
      current_height--;
      current_bin_idx--;
      back_count++;

      if (target_bin_idx < -1 || current_bin_idx < -1) {
        logger::ERROR("URBP, Something error in move_head() - Find common ancestor(1)");
        return; // 나중에 예외처리 한 번 더 살필 것
      }
    }

    if (target_bin_idx == -1 && current_bin_idx == -1) {
      if (target_queue_idx != 0 || current_queue_idx != 0) {
        logger::ERROR("URBP, Something error in move_head() - Find common ancestor(2)");
        return;
      }
    } else if (m_block_pool[target_bin_idx][target_queue_idx].block.getBlockId() !=
               m_block_pool[current_bin_idx][current_queue_idx].block.getBlockId()) {
      logger::ERROR("URBP, Something error in move_head() - Find common ancestor(2)");
      return;
    }

    current_height = static_cast<int>(m_head_height);
    current_bin_idx = m_head_bin_idx;
    current_queue_idx = m_head_queue_idx;

    for (int i = 0; i < back_count; i++) {
      current_queue_idx = m_block_pool[current_bin_idx][current_queue_idx].prev_vector_idx;
      current_bin_idx--;
      current_height--;

      // TODO: Something in ledger process, state tree process
    }

    for (auto &deque_front : where_to_go) {
      if (current_bin_idx + 1 != deque_front.first) {
        logger::ERROR("URBP, Something height error in move_head() - go front");
        return;
      }
      current_bin_idx = deque_front.first;
      current_queue_idx = deque_front.second;
      current_height++; // = m_block_pool[current_bin_idx][current_queue_idx].block.getHeight()

      m_block_pool[current_bin_idx][current_queue_idx];
      // TODO: Something in ledger process, state tree process
    }

    m_head_id = m_block_pool[m_head_bin_idx][m_head_queue_idx].block.getBlockId();
    m_head_height = current_height;
    m_head_bin_idx = current_bin_idx;
    m_head_queue_idx = current_queue_idx;

    if (m_block_pool[m_head_bin_idx][m_head_queue_idx].block.getHeight() != m_head_height) {
      logger::ERROR("URBP, Something error in move_head() - end part, check height");
      return;
    }

    // TODO: missing link 등에 대한 예외처리를 추가해야 할 필요 있음
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

void UnresolvedBlockPool::setupStateTree() // RDB에 있는 모든 노드를 불러올 수 있어야 하므로 관련 연동 필요
{}

// ------------------------------------------------------------------
//
//// block 을 한줄씩 읽으며 데이터와 머클트리 갱신, m_current_layer 갱신
// void KvController::parseBlockToLayer(Block block) {
//  int res;
//  Value value;
//  cout << "parseBlockToLayer function..." << endl;
//  for (auto transaction : block.m_transaction) {
//    cout << transaction << endl;
//    if (transaction["command"] == "add") {
//      res = addCommand(transaction, value);
//    } else if (transaction["command"] == "send") {
//      res = sendCommand(transaction);
//
//    } else if (transaction["command"] == "new") {
//      res = newCommand(transaction);
//    } else if (transaction["command"] == "del") {
//      res = delCommand(transaction);
//    } else {
//      cout << "[ERROR] KvController::parseBlockToLayer - can't analyze transaction - " << transaction << endl;
//      continue;
//    }
//
//    // command 성공 시
//    if (!res) {
//      cout << transaction << " Success." << endl;
//    } else if (res == COIN_VALUE) {
//      cout << transaction << " Failed. command make value under zero." << endl;
//    }
//  }
//  //            m_current_layer.m_layer_tree;
//  //            m_current_layer.m_temporary_data;
//  //            m_current_layer.transaction;
//}
//
// int KvController::addCommand(json transaction, Value &val) {
//  int status = SUCCESS;
//  string block_id = transaction["block_id"];
//  string to_user_id = transaction["to_user_id"];
//  string to_var_type = transaction["to_var_type"];
//  string to_var_name = transaction["to_var_name"];
//  int value = transaction["value"];
//  bool isCurrent = false;
//
//  Key key(block_id, to_user_id, to_var_type, to_var_name);
//  map<Key, Value>::iterator it;
//  int depth = checkLayer(key);
//
//  test_data modified_data;
//
//  if (depth == NO_DATA) {
//    cout << "[ERROR] KvController::addCommand() - Can't find data" << endl;
//    status = DATA_NOT_EXIST;
//  } else {
//    if (depth == DB_DATA) {
//      // DB 연결해서 데이터 읽어온 뒤, add 명령어 반영한 데이터를 m_current_layer 에  저장
//      cout << key << " in the DB" << endl;
//      pair<int, vector<string>> data = m_server.selectAllUsingUserIdVarTypeVarName(to_user_id, to_var_type, to_var_name);
//      val.var_value = data.second[VAR_VALUE];
//      val.path = (uint)stoul(data.second[PATH]);
//      val.isDeleted = false;
//    } else if (depth == CUR_DATA) {
//      // m_current_layer 에 있는 데이터를 읽어온 뒤, add 명령어 반영한 데이터를 다시 m_current_layer 에 저장
//      cout << key << " in the m_current_layer" << endl;
//
//      it = m_current_layer.m_temporary_data.find(key);
//      val = it->second;
//
//      isCurrent = true;
//
//    } else {
//      // m_layer[depth] 에 있는 데이터를 읽어온 뒤, add 명령어 반영한 데이터를 m_current_layer 에 저장
//      cout << key << " in the m_layer[" << depth << "]" << endl;
//
//      it = m_layer[depth].m_temporary_data.find(key);
//      val = it->second;
//    }
//    // Q. add 명령어이면 무조건 var_value 는 정수형인가?
//    val.var_value = to_string(stoi(val.var_value) + value);
//
//    if (stoi(val.var_value) < 0) {
//      val.var_value = to_string(stoi(val.var_value) - value);
//      status = COIN_VALUE;
//    } else {
//      // modified_data.record_id = data.first;
//      modified_data.user_id = key.user_id;
//      modified_data.var_type = key.var_type;
//      modified_data.var_name = key.var_name;
//      modified_data.var_value = val.var_value;
//
//      m_tree.modifyNode(val.path, modified_data);         // merkle tree 의 노드 수정
//      m_current_layer.transaction.push_back(transaction); // 반영된 transaction 보관
//      if (isCurrent) // current layer 에 존재하던 데이터라면, current layer 의 데이터를 수정
//        it->second.var_value = val.var_value;
//      else // DB, m_layer[depth] 에 존재하던 데이터라면, current layer 에 수정된 데이터를 삽입
//        m_current_layer.m_temporary_data.insert(make_pair(key, val));
//    }
//  }
//
//  return status;
//}
//
// int KvController::sendCommand(json transaction) {
//  int status = SUCCESS;
//  string block_id = transaction["block_id"];
//  string to_user_id = transaction["to_user_id"];
//  string to_var_type = transaction["to_var_type"];
//  string to_var_name = transaction["to_var_name"];
//  string from_user_id = transaction["from_user_id"];
//  string from_var_type = transaction["to_var_type"];
//  string from_var_name = transaction["from_var_name"];
//  int value = transaction["value"];
//
//  // send 명령어로 - 값 전송이 올 수 있나??
//  if (value < 0) {
//    cout << "[ERROR] KvController::sendCommand() - Value is under zero" << endl;
//    return COIN_VALUE;
//  }
//
//  Key to_key(to_user_id, to_var_type, to_var_name);
//  Value to_val;
//
//  Key from_key(from_user_id, from_var_type, from_var_name);
//  Value from_val;
//
//  int to_depth = checkLayer(to_key);
//  int from_depth = checkLayer(from_key);
//
//  if (to_depth == NO_DATA) {
//    cout << "[ERROR] KvController::sendCommand() - Can't find to_user data" << endl;
//    status = DATA_NOT_EXIST;
//  } else if (from_depth == NO_DATA) {
//    cout << "[ERROR] KvController::sendCommand() - Can't find from_user data" << endl;
//    status = DATA_NOT_EXIST;
//  } else {
//
//    /* 직접 하는것 보다, addCommand 에 예외처리가 잘 되어있으므로
//     * from user 에 대해서 add 호출, to user 에 대해서 add 호출이 나을 것 같음
//     * (코드도 깔끔해지므로...) */
//    json new_transaction;
//
//    // from 먼저
//    new_transaction["command"] = "add";
//    new_transaction["block_id"] = block_id;
//    new_transaction["to_user_id"] = from_user_id;
//    new_transaction["to_var_type"] = from_var_type;
//    new_transaction["to_var_name"] = from_var_name;
//    new_transaction["value"] = -value;
//
//    status = addCommand(new_transaction, from_val);
//
//    if (!status) {
//      // from 성공하고 나면 to 수행
//      new_transaction["command"] = "add";
//      new_transaction["block_id"] = block_id;
//      new_transaction["to_user_id"] = to_user_id;
//      new_transaction["to_var_type"] = to_var_type;
//      new_transaction["to_var_name"] = to_var_name;
//      new_transaction["value"] = value;
//
//      status = addCommand(new_transaction, to_val);
//    }
//  }
//
//  return status;
//}
//
// int KvController::newCommand(json transaction) {
//  int status = SUCCESS;
//  string block_id = transaction["block_id"];
//  string user_id = transaction["user_id"];
//  string var_type = transaction["var_type"];
//  string var_name = transaction["var_name"];
//  string value = transaction["var_value"];
//
//  if (var_type == "coin" && stoi(value) < 0) {
//    cout << "[ERROR] KvController::newCommand() - Coin type can't minus value" << endl;
//    status = COIN_VALUE;
//  } else {
//    Key key(block_id, user_id, var_type, var_name);
//
//    int depth = checkLayer(key);
//
//    if (depth == NO_DATA) {
//      // new_layer 의 map 변수에 새로운 데이터 삽입
//      uint path = m_tree.getRoot()->makePath(user_id, var_type, var_name);
//      Value val(value, path);
//
//      test_data new_data;
//      new_data.user_id = user_id;
//      new_data.var_type = var_type;
//      new_data.var_name = var_name;
//      new_data.var_value = value;
//
//      m_tree.addNode(path, new_data);
//      m_current_layer.m_temporary_data.insert(make_pair(key, val));
//      cout << key << ", " << val << endl;
//    } else {
//      cout << "[ERROR] KvController::newCommand() - Data already exist" << endl;
//      status = DATA_DUPLICATE;
//    }
//  }
//
//  return status;
//}
//
// int KvController::delCommand(json transaction) {
//  int status = SUCCESS;
//  string block_id = transaction["block_id"];
//  string user_id = transaction["user_id"];
//  string var_type = transaction["var_type"];
//  string var_name = transaction["var_name"];
//  bool isCurrent = false;
//
//  Key key(block_id, user_id, var_type, var_name);
//  Value val;
//  map<Key, Value>::iterator it;
//
//  int depth = checkLayer(key);
//
//  if (depth == NO_DATA) {
//    cout << "[ERROR] KvController::delCommand() - Can't find data" << endl;
//    status = DATA_NOT_EXIST;
//    return status;
//  } else if (depth == DB_DATA) {
//    pair<int, vector<string>> data = m_server.selectAllUsingUserIdVarTypeVarName(user_id, var_type, var_name);
//    val.var_value = data.second[VAR_VALUE];
//    val.path = (uint)stoul(data.second[PATH]);
//  } else if (depth == CUR_DATA) {
//    it = m_current_layer.m_temporary_data.find(key);
//    val = it->second;
//    isCurrent = true;
//  } else {
//    it = m_layer[depth].m_temporary_data.find(key);
//    val = it->second;
//  }
//
//  val.isDeleted = true;
//  m_tree.removeNode(val.path);
//  if (isCurrent) // current layer 에 존재할 경우, current layer 의 데이터 수정
//    it->second.isDeleted = true;
//  else // DB에 있거나 m_layer[depth] 에 존재했을 경우 -> current layer 에 삽입
//    m_current_layer.m_temporary_data.insert(make_pair(key, val));
//
//  return status;
//}
//
//// 현재 레이어부터 시작해서, 윗 레이어부터 살펴보며 데이터가 존재하면 존재하는 레이어 층을 반환함.
//// 레이어에 없으면 마지막으로 DB를 살펴본 뒤, 존재하면 -1, 존재하지 않으면 -2 반환함.
// int KvController::checkLayer(Key key) {
//  int depth = _D_CUR_LAYER;
//  map<Key, Value>::iterator it;
//
//  it = m_current_layer.m_temporary_data.find(key);
//  if (it != m_current_layer.m_temporary_data.end()) {
//    if (it->second.isDeleted) {
//      return NO_DATA;
//    } else {
//      return CUR_DATA;
//    }
//  }
//
//  depth = (int)m_layer.size() - 1;
//
//  for (; depth >= 0; depth--) {
//    it = m_layer[depth].m_temporary_data.find(key);
//    if (it != m_layer[depth].m_temporary_data.end()) {
//      if (it->second.isDeleted) {
//        return NO_DATA;
//      } else {
//        break;
//      }
//    }
//  }
//  if (depth == DB_DATA) {
//    // DB 체크
//    if (m_server.checkUserIdVarTypeVarName(&key.user_id, &key.var_type, &key.var_name)) {
//      // DB 에도 없으면 -2 반환
//      depth = NO_DATA;
//    } else {
//      // 나중에 DB에 lifetime 이 추가되면, isDeleted 역할을 하는 것 처럼 사용할 수도 있을듯...
//    }
//  }
//  return depth;
//}
//
// void KvController::testBackward() {
//  cout << "--------------- test Backward called -----------------" << endl;
//  Layer back_layer = popBackLayer();
//
//  for (auto data : back_layer.m_temporary_data) {
//    Key key = data.first;
//    Value value = data.second;
//    uint path;
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
//          path = (uint)stoul(db_data.second[PATH]);
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
