#ifndef GRUUT_PUBLIC_MERGER_MEM_LEDGER_HPP
#define GRUUT_PUBLIC_MERGER_MEM_LEDGER_HPP

#include "../../../../lib/gruut-utils/src/bytes_builder.hpp"
#include "../../../../lib/gruut-utils/src/sha256.hpp"
#include "../../../../lib/log/include/log.hpp"
#include "../config/storage_type.hpp"

#include <iomanip>
#include <memory>
#include <mutex>
#include <sstream>
#include <stack>
#include <stdlib.h>
#include <string>
#include <vector>

namespace gruut {

// TODO: define 값 변경
#define _TREE_DEPTH 16
#define _SHA256_SPLIT 15

string toHex(int num);
string valueToStr(vector<uint8_t> value); // uint32_t 값을 받아서 눈으로 볼 수 있게 binary string 으로 변환하는 함수
char *intToBin(uint32_t num);

// Debugging
vector<uint8_t> makeValueDebug(string key);
vector<uint8_t> getHash(string l_value, string r_value);

ostream &operator<<(ostream &os, vector<uint8_t> &value);

class StateNode {
private:
  shared_ptr<StateNode> m_left;
  shared_ptr<StateNode> m_right;
  uint32_t m_suffix; // TODO: 비트 확장
  vector<uint8_t> m_value;
  uint32_t m_debug_path; // 노드 분기 시 필요할 것으로 예상됨
  int m_debug_uid;       // 테스트 용도
  int m_suffix_len;

  unique_ptr<user_ledger_type> m_user_ledger;  // 각 leaf node는 하나의 ledger만을 가리킨다
  unique_ptr<contract_ledger_type> m_contract_ledger;

  void makeValue(LedgerRecord &ledger);
  void makeValue(string &key);
  uint32_t makePath(LedgerRecord &ledger);

public:
  StateNode() = default;
  StateNode(LedgerRecord &ledger);
  void reHash();
  void reHash(string l_value, string r_value);

  // 노드 삭제 시 부모 노드로 이동하여 변경되는 path 와 suffix, suffix_len 의 내용을 반영
  // 1. path 의 LSB 에서 suffix_len 번째 비트를 저장
  // 2. suffix 의 LSB 에서 suffix_len 번째 비트에 저장된 비트를 넣고 suffix_len 1 증가
  void moveToParent();
  bool isDummy();
  // bool isLeaf()   { return ((m_debug_uid != -1) && (m_suffix_len == 0)); }

  /* setter */
  void setLeft(shared_ptr<StateNode> node);
  void setRight(shared_ptr<StateNode> node);
  void setSuffix(uint32_t _path, int pos);
  void setDebugPath(uint32_t _path);
  void setNodeInfo(LedgerRecord &data);
  void overwriteNode(shared_ptr<StateNode> node);

  /* getter */
  shared_ptr<StateNode> getLeft() {
    return m_left;
  }
  shared_ptr<StateNode> getRight() {
    return m_right;
  }
  uint32_t getSuffix() {
    return m_suffix;
  }
  vector<uint8_t> getValue() {
    return m_value;
  }
  uint32_t getDebugPath() {
    return m_debug_path;
  }
  // int getDebugUid()       { return m_debug_uid; }
  int getSuffixLen() {
    return m_suffix_len;
  }
};

class StateTree {
private:
  shared_ptr<StateNode> root;
  uint64_t m_size;
  stack<shared_ptr<StateNode>> stk; // leaf 에서 root 까지의 해쉬 재 계산을 위해 StateNode *를 기억해두는 변수

  bool _debug_dir;
  int _debug_depth;
  string _debug_str_depth;
  string _debug_str_dir;

  // LSB 에서 pos 번째 bit 를 반환
  // 반환 값이 false 면 left 방향이고 true 면 right 방향임
  bool getDirectionOf(uint32_t path, int pos);
  void visit(shared_ptr<StateNode> node, bool isPrint);

  // tree post-order 순회 재귀함수
  void postOrder(shared_ptr<StateNode> node, bool isPrint = true);

public:
  StateTree() {
    root = shared_ptr<StateNode>();
    m_size = 0;
  }

  void addNode(uint32_t new_path, shared_ptr<StateNode> node);
  void modifyNode(uint32_t path, LedgerRecord &data);
  void removeNode(uint32_t path);
  shared_ptr<StateNode> getMerkleNode(uint32_t _path);
  vector<vector<uint8_t>> getSiblings(uint32_t _path);

  // Debugging
  void printTreePostOrder();
  // setter
  // void setSize(uint64_t _size) { m_size = _size; }

  uint64_t getSize() {
    return m_size;
  }
  vector<uint8_t> getRootValue() {
    return root->getValue();
  }
  string getRootValueStr() {
    return valueToStr(root->getValue());
  }
  shared_ptr<StateNode> getRoot() {
    return root;
  }
  stack<shared_ptr<StateNode>> getStack() {
    return stk;
  }
};

} // namespace gruut

#endif
