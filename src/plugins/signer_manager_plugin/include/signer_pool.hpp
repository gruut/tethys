#pragma once

#include <algorithm>
#include <mutex>
#include <optional>
#include <random>
#include <string>
#include <unordered_map>
#include <vector>

#include "../../../../lib/gruut-utils/src/random_number_generator.hpp"
#include "../../../../lib/gruut-utils/src/time_util.hpp"
#include "../../../../lib/gruut-utils/src/type_converter.hpp"
#include "signer_manager_plugin.hpp"

namespace gruut {

using namespace std;

constexpr int MAX_SIGNER = 200;
enum class SignerStatus { UNKNOWN, TEMPORARY, ERROR, GOOD };

struct Signer {
  string user_id;
  string pk_cert;
  string hmac_key;
  uint64_t join_start_time;
  string merger_nonce;
  SignerStatus status{SignerStatus::UNKNOWN};
};

class SignerPool {
public:
  SignerPool() = default;
  SignerPool(const SignerPool &) = delete;
  SignerPool(const SignerPool &&) = delete;
  SignerPool &operator=(const SignerPool &) = delete;

  void pushSigner(const string &b58_user_id) {
    Signer new_signer;
    new_signer.user_id = TypeConverter::decodeBase<58>(b58_user_id);
    new_signer.join_start_time = TimeUtil::nowBigInt();
    new_signer.status = SignerStatus::TEMPORARY;

    lock_guard<std::mutex> guard(push_mutex);
    signer_pool[b58_user_id] = new_signer;
    push_mutex.unlock();
  }

  void setMergerNonce(const string &b58_user_id) {
    if (signer_pool.count(b58_user_id) > 0) {
      auto random_num = RandomNumGenerator::randomize(32);
      lock_guard<std::mutex> guard(set_mutex);
      signer_pool[b58_user_id].merger_nonce = TypeConverter::encodeBase<64>(random_num);
      set_mutex.unlock();
    }
  }

  bool setPkCert(const string &b58_user_id, const string &pk_cert) {
    if (signer_pool.count(b58_user_id) > 0) {
      lock_guard<std::mutex> guard(set_mutex);
      signer_pool[b58_user_id].pk_cert = pk_cert;
      set_mutex.unlock();
      return true;
    }
    return false;
  }

  bool setHmacKey(const string &b58_user_id, vector<uint8_t> &hmac_key) {
    if (signer_pool.count(b58_user_id) > 0) {
      lock_guard<std::mutex> guard(set_mutex);
      copy(hmac_key.begin(), hmac_key.end(), signer_pool[b58_user_id].hmac_key.begin());
      set_mutex.unlock();
      return true;
    }
    return false;
  }

  bool setStatus(const string &b58_user_id, SignerStatus status) {
    if (signer_pool.count(b58_user_id) > 0) {
      lock_guard<std::mutex> guard(set_mutex);
      signer_pool[b58_user_id].status = status;
      set_mutex.unlock();
      return true;
    }
    return false;
  }

  bool removeSigner(const string &b58_user_id) {
    if (signer_pool.count(b58_user_id) > 0) {
      lock_guard<std::mutex> guard(push_mutex);
      signer_pool.erase(b58_user_id);
      push_mutex.unlock();
      return true;
    }
    return false;
  }

  const size_t size() {
    return signer_pool.size();
  }

  optional<Signer> getSigner(const string &b58_user_id) {
    if (signer_pool.count(b58_user_id) > 0)
      return signer_pool.at(b58_user_id);
    return {};
  }

  optional<string> getHmacKey(const string &b58_user_id) {
    if (signer_pool.count(b58_user_id) > 0)
      return signer_pool[b58_user_id].hmac_key;
    return {};
  }

  optional<string> getPkCert(const string &b58_user_id) {
    if (signer_pool.count(b58_user_id) > 0)
      return signer_pool[b58_user_id].pk_cert;
    return {};
  }

  vector<Signer> getRandomSigners(size_t number) {
    vector<Signer> signers;
    signers.reserve(number);

    for (auto &[_, signer] : signer_pool) {
      if (signer.status == SignerStatus::GOOD)
        signers.emplace_back(signer);
    }
    shuffle(signers.begin(), signers.end(), mt19937(random_device()()));
    return vector<Signer>(signers.begin(), signers.begin() + number);
  }

  bool full() {
    return MAX_SIGNER >= signer_pool.size();
  }

private:
  unordered_map<string, Signer> signer_pool;
  std::mutex push_mutex;
  std::mutex set_mutex;
};

} // namespace gruut