#ifndef GRUUT_PUBLIC_MERGER_STATE_MERKLE_TREE_HPP
#define GRUUT_PUBLIC_MERGER_STATE_MERKLE_TREE_HPP

#include "../../../../lib/tethys-utils/src/sha256.hpp"

#include <iomanip>
#include <sstream>
#include <stack>
#include <stdio.h>
#include <stdlib.h>
#include <vector>

using namespace std;
using hash_t = std::vector<uint8_t>;

// TODO: define 값 변경
#define _TREE_DEPTH 16
#define _SHA256_SPLIT 15
typedef unsigned int uint;
typedef unsigned long long int ullint;

struct test_data {
  // int record_id;
  string user_id;
  string var_type;
  string var_name;
  string var_value;
};

string toHex(int num);
string valueToStr(vector<uint8_t> value); // uint 값을 받아서 눈으로 볼 수 있게 binary string 으로 변환하는 함수
char *intToBin(uint num);

// Debugging
vector<uint8_t> makeValueDebug(string key);
vector<uint8_t> getHash(string l_value, string r_value);

ostream &operator<<(ostream &os, vector<uint8_t> &value);

namespace tethys {

class StateMerkleNode {
private:
  StateMerkleNode *m_left;
  StateMerkleNode *m_right;
  uint m_suffix; // TODO: 비트 확장
  // string m_value;     // TODO: 비트 확장
  vector<uint8_t> m_value;
  uint m_debug_path; // 노드 분기 시 필요할 것으로 예상됨
  int m_debug_uid;   // 테스트 용도
  int m_suffix_len;
  // StateMerkleNode *m_next;

  void makeValue(test_data data);
  void makeValue(string key);
  uint makePath(test_data data);

public:
  StateMerkleNode(test_data data = {"TEST", "TEST", "TEST", "TEST"});
  void reHash();
  void reHash(string l_value, string r_value);

  // 노드 삭제 시 부모 노드로 이동하여 변경되는 path 와 suffix, suffix_len 의 내용을 반영
  // 1. path 의 LSB 에서 suffix_len 번째 비트를 저장
  // 2. suffix 의 LSB 에서 suffix_len 번째 비트에 저장된 비트를 넣고 suffix_len 1 증가
  void moveToParent();
  bool isDummy();
  // bool isLeaf()   { return ((m_debug_uid != -1) && (m_suffix_len == 0)); }

  /* setter */
  void setLeft(StateMerkleNode *node);
  void setRight(StateMerkleNode *node);
  // void setNext(StateMerkleNode *node)  { m_next  = node;}
  void setSuffix(uint _path, int pos);
  void setDebugPath(uint _path);
  void setNodeInfo(test_data data);
  void overwriteNode(StateMerkleNode *node);

  /* getter */
  StateMerkleNode *getLeft() {
    return m_left;
  }
  StateMerkleNode *getRight() {
    return m_right;
  }
  uint getSuffix() {
    return m_suffix;
  }
  vector<uint8_t> getValue() {
    return m_value;
  }
  uint getDebugPath() {
    return m_debug_path;
  }
  // int getDebugUid()       { return m_debug_uid; }
  int getSuffixLen() {
    return m_suffix_len;
  }
  // StateMerkleNode* getNext() { return m_next; }

  // for Debugging (DB에 path 값 초기 세팅용도)
  // uint makePath(int record_id, string user_id, string var_name)
  uint makePath(string user_id, string var_type, string var_name);
};

class StateMerkleTree {
private:
  StateMerkleNode *root;
  ullint m_size;
  stack<StateMerkleNode *> stk; // leaf 에서 root 까지의 해쉬 재 계산을 위해 StateMerkleNode *를 기억해두는 변수

  bool _debug_dir;
  int _debug_depth;
  string _debug_str_depth;
  string _debug_str_dir;

  // LSB 에서 pos 번째 bit 를 반환
  // 반환 값이 false 면 left 방향이고 true 면 right 방향임
  bool getDirectionOf(uint path, int pos);
  void visit(StateMerkleNode *node, bool isPrint);

  // tree post-order 순회 재귀함수
  void postOrder(StateMerkleNode *node, bool isPrint = true);

public:
  StateMerkleTree() {
    root = new StateMerkleNode();
    m_size = 0;
  }

  // TODO: DB 와 연동하여 완성한 이후에는 new_path 파라미터 제거
  void addNode(uint new_path, test_data data);
  void addNode(uint new_path, StateMerkleNode *new_node);
  void modifyNode(uint path, test_data data);
  void removeNode(uint path);
  StateMerkleNode *getMerkleNode(uint _path);
  vector<vector<uint8_t>> getSiblings(uint _path);

  // Debugging
  void printTreePostOrder();
  // setter
  // void setSize(ullint _size) { m_size = _size; }

  // getter
  ullint getSize() {
    return m_size;
  }
  vector<uint8_t> getRootValue() {
    return root->getValue();
  }
  string getRootValueStr() {
    return valueToStr(root->getValue());
  }
  StateMerkleNode *getRoot() {
    return root;
  }
  stack<StateMerkleNode *> getStack() {
    return stk;
  }
};
} // namespace tethys

#endif // WORKSPACE_MERKLE_LIB_HPP
