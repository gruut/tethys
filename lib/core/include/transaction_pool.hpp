#pragma once

#include <shared_mutex>
#include <map>

#include "../../../include/types/transaction.hpp"

namespace core {
class TransactionPool {
public:
  void add(const TransactionMessage &tx);

private:
  std::map<string, TransactionMessage> tx_pool;
  std::shared_mutex pool_mutex;
};

} // namespace core
