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

struct UserRpcInfo {
  void *tag_identity;
  ServerAsyncWriter<Message> *sender;
};

class UserConnTable {
public:
  UserConnTable() = default;

  UserConnTable(const UserConnTable &) = delete;

  UserConnTable &operator=(const UserConnTable &) = delete;

  UserConnTable(UserConnTable &&) = default;

  UserConnTable &operator=(UserConnTable &&) = default;

  ~UserConnTable() = default;

  void setRpcInfo(const string &recv_id_b58, ServerAsyncWriter<Message> *reply_rpc, void *tag) {
    {
      lock_guard<std::mutex> lock(table_mutex);
      user_conn_table[recv_id_b58].sender = reply_rpc;
      user_conn_table[recv_id_b58].tag_identity = tag;
    }
  }

  void eraseRpcInfo(const string &recv_id_b58) {
    {
      lock_guard<std::mutex> lock(table_mutex);
      user_conn_table.erase(recv_id_b58);
    }
  }

  UserRpcInfo getRpcInfo(const string &recv_id_b58) {
    {
      lock_guard<std::mutex> lock(table_mutex);
      UserRpcInfo rpc_info = user_conn_table[recv_id_b58];

      return rpc_info;
    }
  }

private:
  unordered_map<string, UserRpcInfo> user_conn_table;
  std::mutex table_mutex;
};
} // namespace gruut
