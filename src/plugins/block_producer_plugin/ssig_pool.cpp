#include "include/ssig_pool.hpp"
#include <algorithm>

namespace tethys {
void SupportSigPool::add(const SupportSigInfo &ssig_info) {
  unique_lock<shared_mutex> lock(pool_mutex);
  ssig_pool.emplace(ssig_info.supporter_id, ssig_info);
}

vector<SupportSigInfo> SupportSigPool::fetchAll() {
  vector<SupportSigInfo> v;
  {
    shared_lock<shared_mutex> lock(pool_mutex);
    transform(ssig_pool.begin(), ssig_pool.end(), back_inserter(v), [](auto &kv) { return kv.second; });
  }
  clear();
  return v;
}

int SupportSigPool::size() {
  shared_lock<shared_mutex> lock(pool_mutex);
  return ssig_pool.size();
}

void SupportSigPool::clear() {
  unique_lock<shared_mutex> lock(pool_mutex);
  ssig_pool.clear();
}
}