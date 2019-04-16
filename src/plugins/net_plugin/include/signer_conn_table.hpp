#pragma once

#include "../rpc_services/protos/include/signer_service.grpc.pb.h"
#include "../rpc_services/protos/include/signer_service.pb.h"

#include <memory>
#include <mutex>
#include <thread>
#include <tuple>
#include <unordered_map>

namespace gruut {
using namespace grpc;
using namespace grpc_signer;
using namespace std;

struct SignerRpcInfo {
  void *tag_identity;
  ServerAsyncReaderWriter<Request, Identity> *send_msg;
};

class SignerConnTable {
public:
  SignerConnTable() = default;

  SignerConnTable(const SignerConnTable &) = delete;

  SignerConnTable &operator=(const SignerConnTable &) = delete;

  SignerConnTable(SignerConnTable &&) = default;

  SignerConnTable &operator=(SignerConnTable &&) = default;

  ~SignerConnTable() = default;

  void setRpcInfo(const string &recv_id_b58, ServerAsyncReaderWriter<Request, Identity> *reply_rpc, void *tag) {
    lock_guard<std::mutex> lock(m_mutex);
    m_signer_list[recv_id_b58].send_msg = reply_rpc;
    m_signer_list[recv_id_b58].tag_identity = tag;
    m_mutex.unlock();
  }

  void eraseRpcInfo(const string &recv_id_b58) {
    lock_guard<std::mutex> lock(m_mutex);
    m_signer_list.erase(recv_id_b58);
    m_mutex.unlock();
  }

  SignerRpcInfo getRpcInfo(const string &recv_id_b58) {
    lock_guard<std::mutex> lock(m_mutex);
    SignerRpcInfo rpc_info = m_signer_list[recv_id_b58];
    m_mutex.unlock();
    return rpc_info;
  }

private:
  unordered_map<string, SignerRpcInfo> m_signer_list;
  std::mutex m_mutex;
};
} // namespace gruut
