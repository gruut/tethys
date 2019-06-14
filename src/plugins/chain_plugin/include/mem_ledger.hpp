#ifndef GRUUT_PUBLIC_MERGER_MEM_LEDGER_HPP
#define GRUUT_PUBLIC_MERGER_MEM_LEDGER_HPP

#include "../../../../lib/log/include/log.hpp"
#include "../../../../lib/tethys-utils/src/bytes_builder.hpp"
#include "../../../../lib/tethys-utils/src/sha256.hpp"
#include "../config/storage_type.hpp"

#include <iomanip>
#include <iostream>
#include <map>
#include <memory>
#include <mutex>
#include <sstream>
#include <stack>
#include <stdlib.h>
#include <string>
#include <vector>

namespace tethys {

// TODO: define 값 변경
#define _TREE_DEPTH 16
#define _SHA256_SPLIT 15

string toHex(int num);
string valueToStr(vector<uint8_t> value);
char *intToBin(uint32_t num);

ostream &operator<<(ostream &os, vector<uint8_t> &value);

class StateNode {
private:
  // TODO: 메모리 절약을 위해 최대한 멤버 변수를 줄일 것
  string m_pid;
  shared_ptr<StateNode> m_left;
  shared_ptr<StateNode> m_right;
  vector<uint8_t> m_hash_value;
  path_type m_suffix;
  int m_suffix_len;
  path_type m_bin_path;

  shared_ptr<user_ledger_type> m_user_ledger;
  shared_ptr<contract_ledger_type> m_contract_ledger;

  void makeValue(const user_ledger_type &user_ledger);
  void makeValue(const contract_ledger_type &contract_ledger);
  void makeValue(string &key);
//  uint32_t makePath(user_ledger_type &user_ledger);
//  uint32_t makePath(contract_ledger_type &contract_ledger);

public:
  StateNode() = default;
  StateNode(const user_ledger_type &user_ledger);
  StateNode(const contract_ledger_type &contract_ledger);
  void reHash();
  void reHash(string l_value, string r_value);

  void moveToParent();
  bool isDummy();
  path_type calPathFromPid(const string &pid);

  void setLeft(shared_ptr<StateNode> node);
  void setRight(shared_ptr<StateNode> node);
  void setSuffix(const path_type &path, int pos);
  void setBinPath(const string &pid);
  void setNodeInfo(shared_ptr<StateNode> new_node);
  void overwriteNode(shared_ptr<StateNode> node);

  string getPid();
  shared_ptr<StateNode> getLeft();
  shared_ptr<StateNode> getRight();
  path_type getSuffix();
  int getSuffixLen();
  vector<uint8_t> getValue();
  path_type getBinPath();

  const user_ledger_type &getUserLedger() const;
  const contract_ledger_type &getContractLedger() const;
  shared_ptr<user_ledger_type> getUserLedgerPtr() const;
  shared_ptr<contract_ledger_type> getContractLedgerPtr() const;
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

  bool getDirectionOf(const path_type &path, int pos); // false: left, true: right
  void visit(shared_ptr<StateNode> node, bool isPrint);
  void postOrder(shared_ptr<StateNode> node, bool isPrint = true);

public:
  StateTree() {
    root = shared_ptr<StateNode>();
    m_size = 0;
  }

  void updateUserState(const vector<user_ledger_type> &user_ledger_list);
  void updateUserState(const map<string, user_ledger_type> &user_ledger_list);
  void updateContractState(const vector<contract_ledger_type> &contract_ledger_list);
  void updateContractState(const map<string, contract_ledger_type> &contract_ledger_list);
  template <typename T>
  void insertNode(const T &ledger);   // TODO: user_ledger_type과 contract_ledger_type 두 가지로 template를 제한
  void modifyNode(shared_ptr<StateNode> old_node, shared_ptr<StateNode> new_node);
  void removeNode(const string &pid);
  shared_ptr<StateNode> getMerkleNode(const string &pid);
  vector<vector<uint8_t>> getSiblings(const string &pid);

  void printTreePostOrder();

  uint64_t getSize();
  vector<uint8_t> getRootValue();
  shared_ptr<StateNode> getRootPtr();
  stack<shared_ptr<StateNode>> getStack();
};

} // namespace tethys

#endif
