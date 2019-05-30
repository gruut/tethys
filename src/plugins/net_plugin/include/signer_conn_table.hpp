#pragma once

#include "../rpc_services/protos/include/user_service.grpc.pb.h"
#include "../rpc_services/protos/include/user_service.pb.h"

#include <memory>
#include <mutex>
#include <thread>
#include <tuple>
#include <unordered_map>

namespace gruut {
using namespace grpc;
using namespace grpc_user;
using namespace std;

struct SignerRpcInfo {
  void *tag_identity;
  ServerAsyncWriter<Message> *send_msg;
};

class SignerConnTable {
public:
  SignerConnTable() = default;

  SignerConnTable(const SignerConnTable &) = delete;

  SignerConnTable &operator=(const SignerConnTable &) = delete;

  SignerConnTable(SignerConnTable &&) = default;

  SignerConnTable &operator=(SignerConnTable &&) = default;

  ~SignerConnTable() = default;

  void setRpcInfo(const string &recv_id_b58, ServerAsyncWriter<Message> *reply_rpc, void *tag) {
    {
      lock_guard<std::mutex> lock(table_mutex);
      signer_conn_table[recv_id_b58].send_msg = reply_rpc;
      signer_conn_table[recv_id_b58].tag_identity = tag;
    }
  }

  void eraseRpcInfo(const string &recv_id_b58) {
    {
      lock_guard<std::mutex> lock(table_mutex);
      signer_conn_table.erase(recv_id_b58);
    }
  }

  SignerRpcInfo getRpcInfo(const string &recv_id_b58) {
    {
      lock_guard<std::mutex> lock(table_mutex);
      SignerRpcInfo rpc_info = signer_conn_table[recv_id_b58];

      return rpc_info;
    }
  }

private:
  unordered_map<string, SignerRpcInfo> signer_conn_table;
  std::mutex table_mutex;
};
} // namespace gruut
