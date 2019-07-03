#include "include/transacton_pool.hpp"

namespace tethys {

void TransactionPool::add(const Transaction &tx) {
  {
    unique_lock<shared_mutex> writerLock(pool_mutex);

    tx_pool.try_emplace(tx.getTxId(), tx);
  }
}

vector<Transaction> TransactionPool::fetchAll() {
  {
    shared_lock<shared_mutex> readerLock(pool_mutex);

    vector<Transaction> v;
    v.reserve(tx_pool.size());

    std::transform(tx_pool.begin(), tx_pool.end(), std::back_inserter(v), [](auto &kv) { return kv.second; });

    return v;
  }
}

}