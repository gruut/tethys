#pragma once
#include <shared_mutex>
#include <string>
#include <unordered_map>
#include <vector>

namespace tethys {

using namespace std;
using base58_id_type = string;
using signature_type = string;
using cert_type = string;

struct SupportSigInfo {
  base58_id_type id;
  signature_type sig;
  cert_type cert;
};

class SupportSigPool {
public:
  SupportSigPool() = default;

  void add(const SupportSigInfo &ssig_info);
  vector<SupportSigInfo> fetchAll();
  int size();
  void clear();

private:
  std::shared_mutex pool_mutex;
  unordered_map<base58_id_type, SupportSigInfo> ssig_pool;
};
}