#include "include/mem_ledger.hpp"

string toHex(int num) {
  string str_hex;
  stringstream ss;
  ss << setfill('0') << setw(2) << hex << num;
  ss >> str_hex;
  return str_hex;
}

string valueToStr(vector<uint8_t> value) {
  string str = "";
  for (auto v : value) {
    str += toHex(v);
  }
  return str;
}

// ostream& operator<<(ostream &os, vector<uint8_t> &value)
//{
//    os << valueToStr(value);
//    return os;
//}

namespace tethys {

StateNode::StateNode(const user_ledger_type &user_ledger) {
  m_left = nullptr;
  m_right = nullptr;
  m_suffix = 0;
  m_suffix_len = -1;
  m_bin_path = 0;
  makeValue(user_ledger);
  // m_path = makePath(ledger);

  m_user_ledger = make_shared<user_ledger_type>(user_ledger);
}

StateNode::StateNode(const contract_ledger_type &contract_ledger) {
  m_left = nullptr;
  m_right = nullptr;
  m_suffix = 0;
  m_suffix_len = -1;
  m_bin_path = 0;
  makeValue(contract_ledger);
  // m_path = makePath(ledger);

  m_contract_ledger = make_shared<contract_ledger_type>(contract_ledger);
}

void StateNode::makeValue(const user_ledger_type &user_ledger) {
  BytesBuilder state_value_builder;
  state_value_builder.append(user_ledger.pid);
  state_value_builder.append(user_ledger.var_val);

  string key = state_value_builder.getString();
  makeValue(key);
}

void StateNode::makeValue(const contract_ledger_type &contract_ledger) {
  BytesBuilder state_value_builder;
  state_value_builder.append(contract_ledger.pid);
  state_value_builder.append(contract_ledger.var_val);

  string key = state_value_builder.getString();
  makeValue(key);
}

void StateNode::makeValue(string &key) {
  m_hash_value = Sha256::hash(key);
}

uint32_t StateNode::makePath(LedgerRecord &ledger) {
  // TODO: pid가 주어지지 않았을 때 구현
  // pid가 존재할 때
  string value = valueToStr(Sha256::hash(ledger.pid));
  value = value.substr(value.length() - _SHA256_SPLIT - 1, value.length());
  uint32_t path = (uint32_t)strtoul(value.c_str(), 0, 16);
  uint32_t mask = (1 << _TREE_DEPTH) - 1;

  return path & mask;
}

void StateNode::reHash() {
  string l_value = "", r_value = "";

  if (getLeft() != nullptr) {
    l_value = TypeConverter::encodeBase<64>(getLeft()->getValue());
  }
  if (getRight() != nullptr) {
    r_value = TypeConverter::encodeBase<64>(getRight()->getValue());
  }

  reHash(l_value, r_value);
}

void StateNode::reHash(string l_value, string r_value) {
  if (l_value == "")
    makeValue(r_value);
  else if (r_value == "")
    makeValue(l_value);
  else {
    string combined_value = l_value + r_value;
    makeValue(combined_value);
  }
}

// 노드 삭제 시 부모 노드로 이동하여 변경되는 path 와 suffix, suffix_len 의 내용을 반영
// 1. path 의 LSB 에서 suffix_len 번째 비트를 저장
// 2. suffix 의 LSB 에서 suffix_len 번째 비트에 저장된 비트를 넣고 suffix_len 1 증가
void StateNode::moveToParent() {
  int bit;
  // 예외 처리
  if (m_suffix_len == _TREE_DEPTH) {
    logger::ERROR("StateNode::moveToParent has some error");
    return;
  }
  path_type tmp = m_bin_path;
  tmp &= (1 << m_suffix_len);

  bit = tmp != 0 ? 1 : 0;
  m_suffix |= (bit << m_suffix_len);
  ++m_suffix_len;
}

bool StateNode::isDummy() {
  return (m_bin_path == 0);
}

path_type StateNode::calPathFromPid(const string &pid) {
  vector<bool> bool_vector;
  path_type bin_path;

  for (auto i = pid.rbegin(); i != pid.rend(); ++i) {
    if (i == pid.rbegin() && *i == 0)
      continue;
    bool_vector.emplace_back(*i & 0x01);
    bool_vector.emplace_back(*i & 0x02);
    bool_vector.emplace_back(*i & 0x04);
    bool_vector.emplace_back(*i & 0x08);
    bool_vector.emplace_back(*i & 0x10);
    bool_vector.emplace_back(*i & 0x20);
    bool_vector.emplace_back(*i & 0x40);
    bool_vector.emplace_back(*i & 0x80);
  }

  for (int i = 0; i < bool_vector.size(); ++i) {
    bin_path[i] = bool_vector[i];
  }

  return bin_path;
}

void StateNode::setLeft(shared_ptr<StateNode> node) {
  m_left = node;
}

void StateNode::setRight(shared_ptr<StateNode> node) {
  m_right = node;
}

void StateNode::setSuffix(const path_type &path, int pos) {
  path_type mask;
  for (int i = 0; i < pos; ++i) {
    mask[i] = 1;
  }

  path_type tmp = path;
  tmp &= mask;
  m_suffix = tmp;
  m_suffix_len = pos;
}

void StateNode::setBinPath(const string &pid) {
  m_bin_path = calPathFromPid(pid);
}

void StateNode::setNodeInfo(LedgerRecord &data) {
  makeValue(data);
}

void StateNode::overwriteNode(shared_ptr<StateNode> node) {
  m_left = nullptr;
  m_right = nullptr;
  m_hash_value = node->getValue();
  m_suffix = node->getSuffix();
  m_suffix_len = node->getSuffixLen();
  m_bin_path = node->getBinPath();
  m_user_ledger = node->getUserLedgerPtr();
  m_contract_ledger = node->getContractLedgerPtr();
  moveToParent(); // suffix, suffix_len, path 값 수정

  node.reset();
}

string StateNode::getPid() {
  return m_pid;
}

shared_ptr<StateNode> StateNode::getLeft() {
  return m_left;
}

shared_ptr<StateNode> StateNode::getRight() {
  return m_right;
}

path_type StateNode::getSuffix() {
  return m_suffix;
}

int StateNode::getSuffixLen() {
  return m_suffix_len;
}

vector<uint8_t> StateNode::getValue() {
  return m_hash_value;
}

path_type StateNode::getBinPath() {
  return m_bin_path;
}

const user_ledger_type &StateNode::getUserLedger() const {
  return *m_user_ledger.get();
}

const contract_ledger_type &StateNode::getContractLedger() const {
  return *m_contract_ledger.get();
}

shared_ptr<user_ledger_type> StateNode::getUserLedgerPtr() const {
  return m_user_ledger;
}

shared_ptr<contract_ledger_type> StateNode::getContractLedgerPtr() const {
  return m_contract_ledger;
}

// false: left, true: right
bool StateTree::getDirectionOf(const path_type &path, int pos) {
  path_type tmp = path;
  tmp &= (1 << pos);
  return tmp != 0;
}

void StateTree::visit(shared_ptr<StateNode> node, bool isPrint) {
  string str_dir = !_debug_dir ? "Left" : "Right";
  if (!node->isDummy()) {
    if (isPrint) {
      printf("%s%s\t", _debug_str_dir.substr(0, _debug_depth).c_str(), str_dir.c_str());
      printf("[path: %s] suffix_len: %d, suffix: %s,  hash_value: %s\n", node->getBinPath(), node->getSuffixLen(),
             node->getSuffix(), TypeConverter::encodeBase<64>(node->getValue()).c_str());
    }
    stk.push(node);
  }
  --_debug_depth;
}
// tree post-order 순회 재귀함수
void StateTree::postOrder(shared_ptr<StateNode> node, bool isPrint) {
  if (isPrint) {
    ++_debug_depth;
    string str_dir = (!_debug_dir) ? "Left" : "Right";
    printf("%s[depth %3d] %s\n", _debug_str_depth.substr(0, _debug_depth * 2).c_str(), _debug_depth, str_dir.c_str());
  }
  if (node->getLeft() != nullptr) {
    _debug_dir = false;
    postOrder(node->getLeft(), isPrint);
  }
  if (node->getRight() != nullptr) {
    _debug_dir = true;
    postOrder(node->getRight(), isPrint);
  }
  visit(node, isPrint);
}

template <typename T>
void StateTree::setupTree(const T &ledger_list) {
  for (auto &each_ledger : ledger_list) {
    StateNode new_node(each_ledger);
    this->insertNode(TypeConverter::bytesToString(each_ledger.pid), new_node);
  }
}

template <typename T>
void StateTree::updateState(const T &ledger_list) {
  for (auto &each_ledger : ledger_list) {
    StateNode new_node(each_ledger.second);
    this->insertNode(TypeConverter::bytesToString(each_ledger.second.pid), new_node);
  }
}

void StateTree::updateUserState(const map<string, user_ledger_type> &user_ledger_list) {
  for (auto &each_ledger : user_ledger_list) {
    StateNode new_node(each_ledger.second);
    this->insertNode(TypeConverter::bytesToString(each_ledger.second.pid), make_shared<StateNode>(new_node));
  }
}

void StateTree::updateContractState(const map<string, contract_ledger_type> &contract_ledger_list) {
  for (auto &each_ledger : contract_ledger_list) {
    StateNode new_node(each_ledger.second);
    this->insertNode(TypeConverter::bytesToString(each_ledger.second.pid), make_shared<StateNode>(new_node));
  }
}

void StateTree::insertNode(const string &pid, shared_ptr<StateNode> new_node) {
  new_node->setBinPath(pid);
  path_type new_path = new_node->getBinPath();

  shared_ptr<StateNode> node = root;
  shared_ptr<StateNode> prev_node = nullptr;

  bool collision = false;
  bool dir;
  int dir_pos = _TREE_DEPTH - 1;
  int depth = 1;

  while (!stk.empty())
    stk.pop(); // 스택 clear

  // 머클 루트에서 시작하여 삽입하려는 노드의 경로 (new_path) 를 따라 한칸씩 내려감.
  while (true) {
    // 내려간 노드가 dummy 가 아닌 경우 (데이터를 가진 노드일 경우) 충돌 발샐
    if (!node->isDummy()) {
      collision = true;
      break;
    }
    // 노드 삽입 후 해당 노드에서 머클 루트까지 re-hashing 용도
    stk.push(make_shared<StateNode>(node));

    // 내려가려는 위치의 노드가 nullptr 인 경우, 해당 위치에 노드를 삽입하고 탈출
    dir = getDirectionOf(new_path, dir_pos); // false: left, true: right
    if (!dir && (node->getLeft() == nullptr)) {
      node->setLeft(new_node);
      new_node->setSuffix(new_path, dir_pos);
      break;
    } else if (dir && (node->getRight() == nullptr)) {
      node->setRight(new_node);
      new_node->setSuffix(new_path, dir_pos);
      break;
    }
    // path 로 진행하는 방향에 노드가 존재하면 계속해서 내려감
    else {
      if (!dir) {
        prev_node = node;
        node = node->getLeft();
      } else {
        prev_node = node;
        node = node->getRight();
      }
    }

    --dir_pos;
    ++depth;
  }
  // case: 기존 노드와 충돌 났을 경우 -> 기존 노드와 새 노드 모두 적절한 위치에 삽입 필요
  // prev_node : 부모 노드, dummy : 충돌난 곳에 삽입할 더미 노드
  // old_node : 충돌난 곳에 있던 기존 노드, new_node : 삽입하려는 새 노드
  // old_path : 기존 노드의 경로, new_path : 새 노드의 경로
  if (collision) {
    shared_ptr<StateNode> old_node;
    path_type old_path;
    shared_ptr<StateNode> dummy;

    old_node = node;
    old_path = old_node->getBinPath();

    if (old_node->getPid() == new_node->getPid())
      modifyNode(new_node);

    // 먼저 충돌난 곳에 dummy 노드 생성해서 연결
    dummy = make_shared<StateNode>();
    stk.push(dummy);
    if (!getDirectionOf(new_path, dir_pos + 1))
      prev_node->setLeft(dummy);
    else
      prev_node->setRight(dummy);
    prev_node = dummy;

    // 경로가 같은 횟수만큼 경로를 따라서 dummy 노드 생성 및 연결
    while (true) {
      if (depth > _TREE_DEPTH) {
        printf("StateTree::insertNode() collision unsolved.\n");
        break;
      }

      if (getDirectionOf(new_path, dir_pos) != getDirectionOf(old_path, dir_pos)) {
        break;
      } else {
        dummy = make_shared<StateNode>();
        stk.push(dummy);
        if (!getDirectionOf(new_path, dir_pos)) {
          prev_node->setLeft(dummy);
        } else {
          prev_node->setRight(dummy);
        }
        prev_node = dummy;
      }

      --dir_pos;
      ++depth;
    }
    // 경로가 다른 위치에서 기존 노드, 새 노드 각각 삽입
    if (!getDirectionOf(new_path, dir_pos)) {
      prev_node->setLeft(new_node);
      prev_node->setRight(old_node);
      new_node->setSuffix(new_path, dir_pos);
      old_node->setSuffix(old_path, dir_pos);
    } else {
      prev_node->setRight(new_node);
      prev_node->setLeft(old_node);
      new_node->setSuffix(new_path, dir_pos);
      old_node->setSuffix(old_path, dir_pos);
    }
  } // collision 해결

  // 머클 루트까지 re-hashing
  shared_ptr<StateNode> tmp;
  while (!stk.empty()) {
    tmp = stk.top();
    stk.pop();

    tmp->reHash();
  }

  ++m_size;
}

void StateTree::modifyNode(shared_ptr<StateNode> node) {
  node->setNodeInfo(node);

  // 머클 루트까지 re-hashing
  while (!stk.empty()) {
    node = stk.top();
    stk.pop();

    node->reHash();
  }
}

void StateTree::removeNode(string &pid) {
  shared_ptr<StateNode> node;
  shared_ptr<StateNode> parent, left, right;

  node = getMerkleNode(pid);
  parent = stk.top();
  stk.pop();

  // 해당 노드 삭제
  if (parent->getLeft() == node) {
    parent->setLeft(nullptr);
    node.reset();
  } else if (parent->getRight() == node) {
    parent->setRight(nullptr);
    node.reset();
  }

  // 머클 루트까지 올라가며 노드 정리
  do {
    if (parent == root) {
      parent->reHash();
      break;
    }

    left = parent->getLeft();
    right = parent->getRight();

    if (left != nullptr && right != nullptr)
      break;                     // left, right 가 모두 존재하면 머클 루트까지 rehash 시작
    else if (right != nullptr) { // right 만 존재하면 parent 를 right 로 덮어씌움
      if (!right->isDummy())
        parent->overwriteNode(right);
      else
        break;
    } else if (left != nullptr) { // left 만 존재하면 parent 를 left 로 덮어씌움
      if (!left->isDummy())
        parent->overwriteNode(left);
      else
        break;
    }

    if (stk.empty())
      break;
    else
      parent = stk.top();
    stk.pop();
  } while (true);

  parent->reHash();
  while (!stk.empty()) {
    parent = stk.top();
    stk.pop();
    parent->reHash();
  }
  --m_size;
}

shared_ptr<StateNode> StateTree::getMerkleNode(string &pid) {
  shared_ptr<StateNode> ret = nullptr;
  path_type path = ret->calPathFromPid(pid);

  while (!stk.empty()) {
    stk.pop(); // 스택 clear
  }

  // _path 따라 내려가면서 dummy 가 아닌 노드 (실제 정보를 가진 노드)를 발견하면 반환
  shared_ptr<StateNode> node = root;
  int dir_pos = _TREE_DEPTH - 1;
  while (true) {
    if (!node->isDummy()) {
      ret = node;
      break;
    }

    if (dir_pos < 0) {
      break;
    }
    stk.push(node);
    if (!getDirectionOf(path, dir_pos)) {
      node = node->getLeft();
    } else {
      node = node->getRight();
    }
    --dir_pos;
  }
  return ret;
}

vector<vector<uint8_t>> StateTree::getSiblings(string &pid) {
  shared_ptr<StateNode> node = getMerkleNode(pid);

  vector<vector<uint8_t>> siblings;
  vector<uint8_t> value;

  shared_ptr<StateNode> parent_node;
  siblings.clear();
  while (!stk.empty()) {
    parent_node = stk.top();
    stk.pop();

    value.clear();
    if (parent_node->getLeft() == node) {
      if (parent_node->getRight() != nullptr)
        value = parent_node->getRight()->getValue();
    } else if (parent_node->getRight() == node) {
      if (parent_node->getLeft() != nullptr)
        value = parent_node->getLeft()->getValue();
    }
    // printf("getSiblings::value.size() = %ld\n", value.size());
    siblings.push_back(value);
  }
  return siblings;
}

void StateTree::printTreePostOrder() {
  bool isPrint = true;

  if (isPrint) {
    uint64_t size = getSize();
    printf("*********** print tree data by using post-order traversal... ************\n\n");
    printf("node size: %lld\n\n", getSize());
    if (size == 0) {
      printf("there is nothing to printing!!\n\n");
      return;
    }
    _debug_depth = -1;
    _debug_dir = false;
    _debug_str_depth = _debug_str_dir = "";
    for (int i = 0; i < _TREE_DEPTH * 2; ++i) {
      _debug_str_depth += " ";
      _debug_str_dir += "\t";
    }
  }

  while (!stk.empty()) {
    stk.pop();
  }

  postOrder(root, isPrint);

  if (isPrint) {
        printf("root Value: %s\n", TypeConverter::encodeBase<64>(TypeConverter::bytesToString(root->getValue())));
        printf("*********** finish traversal ***********\n");
  }
}

uint64_t StateTree::getSize() {
  return m_size;
}

vector<uint8_t> StateTree::getRootValue() {
  return root->getValue();
}

shared_ptr<StateNode> StateTree::getRootPtr() {
  return root;
}

stack<shared_ptr<StateNode>> StateTree::getStack() {
  return stk;
}

} // namespace tethys
