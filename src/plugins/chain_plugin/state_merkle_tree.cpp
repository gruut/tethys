#include "include/state_merkle_tree.hpp"

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

// uint 값을 받아서 눈으로 볼 수 있게 binary string 으로 변환하는 함수
char *intToBin(uint num) {
  char *ret = new char(_SHA256_SPLIT + 2);

  int pos = 0;
  for (int i = _SHA256_SPLIT; i >= 0; --i) {
    if ((num & (1 << i)) == 0) {
      ret[pos] = '0';
    } else {
      ret[pos] = '1';
    }
    pos++;
  }
  ret[_SHA256_SPLIT + 1] = '\0';

  return ret;
}

// ostream& operator<<(ostream &os, vector<uint8_t> &value)
//{
//    os << valueToStr(value);
//    return os;
//}

namespace gruut {

void StateMerkleNode::makeValue(test_data data) {
  // string key = to_string(data.record_id) + data.user_id + data.var_name + data.var_type + data.var_value;
  string key = data.user_id + data.var_type + data.var_name + data.var_value;
  makeValue(key);
}
void StateMerkleNode::makeValue(string key) {
  m_value = Sha256::hash(key);
}
uint StateMerkleNode::makePath(test_data data) {
  // string key = to_string(data.record_id) + data.user_id + data.var_name;
  // cout<<"makePath"<<endl;
  string key = data.user_id + data.var_type + data.var_name;
  string value = valueToStr(Sha256::hash(key));
  value = value.substr(value.length() - _SHA256_SPLIT - 1, value.length());
  uint path = (uint)strtoul(value.c_str(), 0, 16);
  uint mask = (1 << _TREE_DEPTH) - 1;

  return path & mask;
}

StateMerkleNode::StateMerkleNode(test_data data) {
  m_left = nullptr;
  m_right = nullptr;
  // m_next = nullptr;
  m_suffix = 0;
  makeValue(data);

  // m_path = makePath(data);
  m_debug_path = 0;
  // m_debug_uid = data.record_id;
  m_suffix_len = -1;
}

void StateMerkleNode::reHash() {
  string l_value = "", r_value = "";

  if (getLeft() != nullptr) {
    l_value = TypeConverter::encodeBase64(getLeft()->getValue());
  }
  if (getRight() != nullptr) {
    r_value = TypeConverter::encodeBase64(getRight()->getValue());
  }

  reHash(l_value, r_value);
}
void StateMerkleNode::reHash(string l_value, string r_value) {
  if (l_value == "")
    makeValue(r_value);
  else if (r_value == "")
    makeValue(l_value);
  else
    makeValue(l_value + r_value);
}

// 노드 삭제 시 부모 노드로 이동하여 변경되는 path 와 suffix, suffix_len 의 내용을 반영
// 1. path 의 LSB 에서 suffix_len 번째 비트를 저장
// 2. suffix 의 LSB 에서 suffix_len 번째 비트에 저장된 비트를 넣고 suffix_len 1 증가
void StateMerkleNode::moveToParent() {
  int bit;
  // 예외 처리
  if (m_suffix_len == _TREE_DEPTH) {
    printf("StateMerkleNode::moveToParent has some error");
    return;
  }

  bit = (m_debug_path & (1 << m_suffix_len)) != 0 ? 1 : 0;
  m_suffix = m_suffix | (bit << m_suffix_len);
  m_suffix_len++;
}

bool StateMerkleNode::isDummy() {
  return (m_debug_path == 0);
}
// bool isLeaf()   { return ((m_debug_uid != -1) && (m_suffix_len == 0)); }

/* setter */
void StateMerkleNode::setLeft(StateMerkleNode *node) {
  m_left = node;
}
void StateMerkleNode::setRight(StateMerkleNode *node) {
  m_right = node;
}
// void setNext(StateMerkleNode *node)  { m_next  = node;}
void StateMerkleNode::setSuffix(uint _path, int pos) {
  uint mask = (uint)(1 << pos) - 1;
  m_suffix = _path & mask;
  m_suffix_len = pos;
}
void StateMerkleNode::setDebugPath(uint _path) {
  m_debug_path = _path;
}
void StateMerkleNode::setNodeInfo(test_data data) {
  // m_debug_uid = data.record_id;
  makeValue(data);
}
void StateMerkleNode::overwriteNode(StateMerkleNode *node) {
  m_left = nullptr;
  m_right = nullptr;
  m_value = node->getValue();
  // m_debug_uid = node->getDebugUid();
  m_suffix = node->getSuffix();
  m_debug_path = node->getDebugPath();
  m_suffix_len = node->getSuffixLen();
  moveToParent(); // suffix, suffix_len, path 값 수정

  delete node;
}

// for Debugging (DB에 path 값 초기 세팅용도)
// uint makePath(int record_id, string user_id, string var_name)
uint StateMerkleNode::makePath(string user_id, string var_type, string var_name) {
  test_data tmp;
  // tmp.record_id   = record_id;
  tmp.user_id = user_id;
  tmp.var_type = var_type;
  tmp.var_name = var_name;
  tmp.var_value = "0";
  uint path = makePath(tmp);
  // printf("path: %u\n", path);
  return path;
}

// LSB 에서 pos 번째 bit 를 반환
// 반환 값이 false 면 left 방향이고 true 면 right 방향임
bool StateMerkleTree::getDirectionOf(uint path, int pos) {
  return (path & (1 << pos)) != 0;
}

void StateMerkleTree::visit(StateMerkleNode *node, bool isPrint) {
  string str_dir = !_debug_dir ? "Left" : "Right";
  if (!node->isDummy()) {
    if (isPrint) {
      printf("%s%s\t", _debug_str_dir.substr(0, _debug_depth).c_str(), str_dir.c_str());
      printf("[path: %s] suffix_len: %d, suffix: %s,  hash_value: %s\n", intToBin(node->getDebugPath()), node->getSuffixLen(),
             intToBin(node->getSuffix()), TypeConverter::encodeBase64(node->getValue()).c_str());
    }
    stk.push(node);
  }
  _debug_depth--;
}
// tree post-order 순회 재귀함수
void StateMerkleTree::postOrder(StateMerkleNode *node, bool isPrint) {
  if (isPrint) {
    _debug_depth++;
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

// TODO: DB 와 연동하여 완성한 이후에는 new_path 파라미터 제거
void StateMerkleTree::addNode(uint new_path, test_data data) {
  StateMerkleNode *node = new StateMerkleNode(data);
  node->setDebugPath(new_path);
  addNode(new_path, node);
}

void StateMerkleTree::addNode(uint new_path, StateMerkleNode *new_node) {
  StateMerkleNode *node = root;
  StateMerkleNode *prev_node = nullptr;

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
    stk.push(node);

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

    dir_pos--;
    depth++;
  }
  // case: 기존 노드와 충돌 났을 경우 -> 기존 노드와 새 노드 모두 적절한 위치에 삽입 필요
  // prev_node : 부모 노드, dummy : 충돌난 곳에 삽입할 더미 노드
  // old_node : 충돌난 곳에 있던 기존 노드, new_node : 삽입하려는 새 노드
  // old_path : 기존 노드의 경로, new_path : 새 노드의 경로
  if (collision) {
    StateMerkleNode *old_node;
    uint old_path;
    StateMerkleNode *dummy;

    old_node = node;
    old_path = old_node->getDebugPath();

    // 먼저 충돌난 곳에 dummy 노드 생성해서 연결
    dummy = new StateMerkleNode();
    stk.push(dummy);
    if (!getDirectionOf(new_path, dir_pos + 1))
      prev_node->setLeft(dummy);
    else
      prev_node->setRight(dummy);
    prev_node = dummy;

    // 경로가 같은 횟수만큼 경로를 따라서 dummy 노드 생성 및 연결
    while (true) {
      if (depth > _TREE_DEPTH) {
        printf("StateMerkleTree::addNode() collision unsolved.\n");
        break;
      }

      if (getDirectionOf(new_path, dir_pos) != getDirectionOf(old_path, dir_pos)) {
        break;
      } else {
        dummy = new StateMerkleNode();
        stk.push(dummy);
        if (!getDirectionOf(new_path, dir_pos)) {
          prev_node->setLeft(dummy);
        } else {
          prev_node->setRight(dummy);
        }
        prev_node = dummy;
      }

      dir_pos--;
      depth++;
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
  StateMerkleNode *tmp;
  while (!stk.empty()) {
    tmp = stk.top();
    stk.pop();

    tmp->reHash();
  }

  m_size++;
}

void StateMerkleTree::modifyNode(uint path, test_data data) {
  StateMerkleNode *node = getMerkleNode(path);

  node->setNodeInfo(data);

  // 머클 루트까지 re-hashing
  while (!stk.empty()) {
    node = stk.top();
    stk.pop();

    node->reHash();
  }
}

void StateMerkleTree::removeNode(uint path) {
  StateMerkleNode *node;
  StateMerkleNode *parent, *left, *right;

  node = getMerkleNode(path);
  parent = stk.top();
  stk.pop();

  // 해당 노드 삭제
  if (parent->getLeft() == node) {
    parent->setLeft(nullptr);
    delete node;
  } else if (parent->getRight() == node) {
    parent->setRight(nullptr);
    delete node;
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
  m_size--;
}

StateMerkleNode *StateMerkleTree::getMerkleNode(uint _path) {
  StateMerkleNode *ret = nullptr;

  while (!stk.empty()) {
    stk.pop(); // 스택 clear
  }

  // _path 따라 내려가면서 dummy 가 아닌 노드 (실제 정보를 가진 노드)를 발견하면 반환
  StateMerkleNode *node = root;
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
    if (!getDirectionOf(_path, dir_pos)) {
      node = node->getLeft();
    } else {
      node = node->getRight();
    }
    dir_pos--;
  }
  return ret;
}

vector<vector<uint8_t>> StateMerkleTree::getSiblings(uint _path) {
  StateMerkleNode *node = getMerkleNode(_path);

  vector<vector<uint8_t>> siblings;
  vector<uint8_t> value;

  StateMerkleNode *parent_node;
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

// Debugging
void StateMerkleTree::printTreePostOrder() {
  bool isPrint = true;

  if (isPrint) {
    ullint size = getSize();
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
    printf("root Value: %s\n", TypeConverter::encodeBase64(root->getValue()).c_str());
    printf("*********** finish traversal ***********\n");
  }
}

// setter
// void setSize(ullint _size) { m_size = _size; }
} // namespace gruut
