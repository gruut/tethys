#pragma once
#include "../structure/transaction.hpp"
#include <map>
#include <shared_mutex>
#include <vector>

namespace tethys {
class TransactionPool {
public:
  void add(const Transaction &tx);
  vector<Transaction> fetchAll();

private:
  std::map<string, Transaction> tx_pool;
  std::shared_mutex pool_mutex;
};
}