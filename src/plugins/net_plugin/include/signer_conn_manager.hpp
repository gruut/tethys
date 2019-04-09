#pragma once

#include "../rpc_services/protos/include/general_service.grpc.pb.h"
#include "../rpc_services/protos/include/general_service.pb.h"

#include <memory>
#include <mutex>
#include <thread>
#include <tuple>
#include <unordered_map>

namespace gruut {
using namespace grpc;
using namespace grpc_general;

struct SignerRpcInfo {
  void *tag_identity;
  ServerAsyncReaderWriter<ReplyMsg, Identity> *send_msg;
};

class SignerConnTable {
public:
  SignerConnTable() = default;

  SignerConnTable(const SignerConnTable &) = delete;

  SignerConnTable &operator=(const SignerConnTable &) = delete;

  SignerConnTable(SignerConnTable &&) = default;

  SignerConnTable &operator=(SignerConnTable &&) = default;

  ~SignerConnTable() = default;

  void setRpcInfo(const std::string &recv_id_b58, ServerAsyncReaderWriter<ReplyMsg, Identity> *reply_rpc, void *tag) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_signer_list[recv_id_b58].send_msg = reply_rpc;
    m_signer_list[recv_id_b58].tag_identity = tag;
    m_mutex.unlock();
  }

  void eraseRpcInfo(const std::string &recv_id_b58) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_signer_list.erase(recv_id_b58);
    m_mutex.unlock();
  }

  SignerRpcInfo getRpcInfo(const std::string &recv_id_b58) {
    std::lock_guard<std::mutex> lock(m_mutex);
    SignerRpcInfo rpc_info = m_signer_list[recv_id_b58];
    m_mutex.unlock();
    return rpc_info;
  }

private:
  std::unordered_map<string, SignerRpcInfo> m_signer_list;
  std::mutex m_mutex;
};
} // namespace gruut
