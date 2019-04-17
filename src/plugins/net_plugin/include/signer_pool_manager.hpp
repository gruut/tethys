#pragma once

#include "../../../../lib/gruut-utils/src/bytes_builder.hpp"
#include "../../../../lib/gruut-utils/src/ecdsa.hpp"
#include "../../../../lib/gruut-utils/src/hmac_key_maker.hpp"
#include "../../../../lib/gruut-utils/src/random_number_generator.hpp"
#include "../../../../lib/gruut-utils/src/time_util.hpp"
#include "../../../../lib/gruut-utils/src/type_converter.hpp"
#include "../../../../lib/json/include/json.hpp"
#include "../../../../lib/log/include/log.hpp"
#include "../config/include/message.hpp"
#include "../config/include/network_config.hpp"
#include "message_builder.hpp"

#include <algorithm>
#include <atomic>
#include <mutex>
#include <optional>
#include <random>
#include <string>
#include <unordered_map>
#include <vector>

namespace gruut {

using namespace net_plugin;
using namespace std;

constexpr uint64_t JOIN_TIMEOUT_SEC = 10;
constexpr int MAX_SIGNER = 200;

struct Signer {
  string user_id;
  vector<uint8_t> hmac_key;
};

struct JoinTempData {
  string merger_nonce;
  string signer_pk_cert;
  vector<uint8_t> shared_sk;
  uint64_t start_time;
};

using TempSignerPool = unordered_map<string, JoinTempData>;

class SignerPool {
public:
  SignerPool() = default;
  SignerPool(const SignerPool &) = delete;
  SignerPool(const SignerPool &&) = delete;
  SignerPool &operator=(const SignerPool &) = delete;

  void pushSigner(const string &b58_user_id, vector<uint8_t> &hmac_key) {
    Signer new_signer;
    new_signer.user_id = TypeConverter::decodeBase<58>(b58_user_id);
    new_signer.hmac_key = hmac_key;

    {
      lock_guard<std::mutex> guard(pool_mutex);
      signer_pool[b58_user_id] = new_signer;
    }
  }

  bool eraseSigner(const string &b58_user_id) {
    {
      lock_guard<std::mutex> guard(pool_mutex);

      if (signer_pool.count(b58_user_id) > 0) {
        signer_pool.erase(b58_user_id);
        return true;
      }
      return false;
    }
  }

  optional<vector<uint8_t>> getHmacKey(const string &b58_user_id) {
    {
      lock_guard<std::mutex> guard(pool_mutex);

      if (signer_pool.count(b58_user_id) > 0)
        return signer_pool[b58_user_id].hmac_key;
      return {};
    }
  }

  bool full() {
    {
      lock_guard<std::mutex> guard(pool_mutex);

      return MAX_SIGNER >= signer_pool.size();
    }
  }

private:
  unordered_map<string, Signer> signer_pool;
  std::mutex pool_mutex;
};

class SignerPoolManager {
public:
  SignerPoolManager() = default;
  SignerPoolManager(const SignerPoolManager &) = delete;
  SignerPoolManager(SignerPoolManager &&) = default;
  SignerPoolManager &operator=(SignerPoolManager &&) = default;

  OutNetMsg handleMsg(InNetMsg &msg) {
    string recv_id_b58 = msg.sender_id;

    switch (msg.type) {
    case MessageType::MSG_JOIN: {
      if (!isJoinable()) {
        logger::ERROR("[ECDH] Signer pool is full");
        throw ErrorMsgType::ECDH_MAX_SIGNER_POOL;
      }

      auto signer_id_in_body = json::get<string>(msg.body, "user");
      if (signer_id_in_body.value() == msg.sender_id) {
        logger::ERROR("[ECDH] Message header and body id are different");
        throw ErrorMsgType::ECDH_ILLEGAL_ACCESS;
      }

      temp_signer_pool[msg.sender_id].start_time = TimeUtil::nowBigInt();
      auto merger_nonce = TypeConverter::encodeBase<64>((RandomNumGenerator::randomize(32)));
      temp_signer_pool[msg.sender_id].merger_nonce = merger_nonce;

      auto msg_challenge = MessageBuilder::build<MessageType::MSG_CHALLENGE>(recv_id_b58, merger_nonce);
      return msg_challenge;
    }
    case MessageType::MSG_RESPONSE_1: {
      if (temp_signer_pool.find(msg.sender_id) == temp_signer_pool.end()) {
        logger::ERROR("[ECDH] Illegal Trial");
        throw ErrorMsgType::ECDH_ILLEGAL_ACCESS;
      }
      if (isTimeout(msg.sender_id)) {
        logger::ERROR("[ECDH] Join timeout");
        throw ErrorMsgType::ECDH_TIMEOUT;
      }
      if (!verifySignature(msg.sender_id, msg.body)) {
        logger::ERROR("[ECDH] Invalid Signature");
        throw ErrorMsgType::ECDH_INVALID_SIG;
      }

      temp_signer_pool[msg.sender_id].signer_pk_cert = json::get<string>(msg.body["user"], "pk").value();

      HmacKeyMaker key_maker;
      auto [dhx, dhy] = key_maker.getPublicKey();

      auto signer_dhx = json::get<string>(msg.body["dh"], "x").value();
      auto signer_dhy = json::get<string>(msg.body["dh"], "y").value();
      auto shared_sk_bytes = key_maker.getSharedSecretKey(signer_dhx, signer_dhy, 32);
      if (shared_sk_bytes.empty()) {
        logger::ERROR("[ECDH] Failed to generate SSK (invalid PK)");
        throw ErrorMsgType::ECDH_INVALID_PK;
      }
      temp_signer_pool[msg.sender_id].shared_sk = shared_sk_bytes;

      auto sn = json::get<string>(msg.body["user"], "sn").value();
      auto mn = temp_signer_pool[msg.sender_id].merger_nonce;
      auto curr_time = TimeUtil::now();
      // TODO : get sk, sk_pass, cert
      string my_sk, my_pass, my_cert;
      auto sig = signMessage(curr_time, mn, sn, dhx, dhy, my_sk, my_pass);
      auto msg_response2 = MessageBuilder::build<MessageType::MSG_RESPONSE_2>(curr_time, recv_id_b58, dhx, dhy, my_cert, sig);

      return msg_response2;
    }
    case MessageType::MSG_SUCCESS: {
      if (isTimeout(msg.sender_id)) {
        logger::ERROR("[ECDH] Join timeout");
        throw ErrorMsgType::ECDH_TIMEOUT;
      }

      signer_pool.pushSigner(recv_id_b58, temp_signer_pool[recv_id_b58].shared_sk);
      temp_signer_pool.erase(recv_id_b58);

      // TODO : signer join event should be notified to `Block producer`

      auto msg_accept = MessageBuilder::build<MessageType::MSG_ACCEPT>(recv_id_b58);
      return msg_accept;
    }
    default: {
      logger::ERROR("[ECDH] Unknown message");
      throw ErrorMsgType::UNKNOWN;
    }
    }
  }

  void removeSigner(const string &b58_signer_id) {
    signer_pool.eraseSigner(b58_signer_id);
  }
  void removeTempSigner(const string &b58_signer_id) {
    if (temp_signer_pool.count(b58_signer_id) > 0)
      temp_signer_pool.erase(b58_signer_id);
  }

  optional<vector<uint8_t>> getHmacKey(const string &b58_signer_id) {
    return signer_pool.getHmacKey(b58_signer_id);
  }

private:
  bool isJoinable() {
    return signer_pool.full();
  }

  bool isTimeout(const string &b58_signer_id) {
    return (TimeUtil::nowBigInt() - temp_signer_pool[b58_signer_id].start_time) > JOIN_TIMEOUT_SEC;
  }

  bool verifySignature(const string &signer_id_b58, nlohmann::json &msg_body) {
    auto dh = json::get<nlohmann::json>(msg_body, "dh");
    auto sn = json::get<string>(msg_body, "sn");
    auto timestamp = json::get<string>(msg_body, "time");
    auto signer_info = json::get<nlohmann::json>(msg_body, "user");
    if (!dh.has_value() || !sn.has_value() || !timestamp.has_value() || !signer_info.has_value())
      return false;

    auto dhx = json::get<string>(dh.value(), "x");
    auto dhy = json::get<string>(dh.value(), "y");
    auto signer_pk = json::get<string>(signer_info.value(), "pk");
    auto signer_b64_sig = json::get<string>(signer_info.value(), "sig");
    if (!dhx.has_value() || !dhy.has_value() || !signer_pk.has_value() || !signer_b64_sig.has_value())
      return false;

    auto signer_sig_str = TypeConverter::decodeBase<64>(signer_b64_sig.value());
    auto mn = temp_signer_pool[signer_id_b58].merger_nonce;

    BytesBuilder bytes_builder;
    bytes_builder.appendBase<64>(mn);
    bytes_builder.appendBase<64>(sn.value());
    bytes_builder.appendHex(dhx.value());
    bytes_builder.appendHex(dhy.value());
    bytes_builder.appendDec(timestamp.value());

    return ECDSA::doVerify(signer_pk.value(), bytes_builder.getBytes(), vector<uint8_t>(signer_sig_str.begin(), signer_sig_str.end()));
  }
  string signMessage(const string &timestamp, const string &mn, const string &sn, const string &dhx, const string &dhy,
                     const string &sk_pem, const string &sk_pass) {
    BytesBuilder bytes_builder;
    bytes_builder.appendBase<64>(mn);
    bytes_builder.appendBase<64>(sn);
    bytes_builder.appendHex(dhx);
    bytes_builder.appendHex(dhy);
    bytes_builder.appendDec(timestamp);

    auto sig = ECDSA::doSign(sk_pem, bytes_builder.getBytes(), sk_pass);
    return TypeConverter::encodeBase<64>(sig);
  }

  SignerPool signer_pool;
  TempSignerPool temp_signer_pool;
};

} // namespace gruut