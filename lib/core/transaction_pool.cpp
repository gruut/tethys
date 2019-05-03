#include "include/transaction_pool.hpp"

using namespace std;

namespace core {
void TransactionPool::add(const TransactionMessage &tx) {
  {
    lock_guard<shared_mutex> writerLock(pool_mutex);
    tx_pool.try_emplace(tx.txid, tx);
  }
}
} // namespace core
