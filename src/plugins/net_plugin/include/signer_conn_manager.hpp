#pragma once

#include "rpc_services/rpc_services.hpp"
#include <memory>
#include <mutex>
#include <thread>
#include <tuple>
#include <unordered_map>

using namespace grpc;
using namespace grpc_general;
namespace gruut {
namespace net {

struct SignerRpcInfo {
  void *tag_identity;
  ServerAsyncReaderWriter <ReplyMsg, Identity> *send_msg;
};

class SignerConnTable {
public:
  SignerConnTable() : m_signer_cnt{0} {}

  SignerConnTable(const SignerConnTable &) = delete;
  SignerConnTable &operator=(const SignerConnTable &) = delete;
  SignerConnTable (SignerConnTable &&) = default;
  SignerConnTable &operator=(SignerConnTable &&) = default;
  ~SignerConnTable() = default;

  void setReplyMsg(std::string &recv_id_b64,
				   ServerAsyncReaderWriter <ReplyMsg, Identity> *reply_rpc,
				   void *tag) {
	std::lock_guard<std::mutex> lock(m_mutex);
	m_signer_list[recv_id_b64].send_msg = reply_rpc;
	m_signer_list[recv_id_b64].tag_identity = tag;
	m_mutex.unlock();
  }

  void eraseRpcInfo(std::string &recv_id_b64) {
	std::lock_guard<std::mutex> lock(m_mutex);
	m_signer_list.erase(recv_id_b64);
	m_signer_cnt--;
	m_mutex.unlock();
  }

  SignerRpcInfo getRpcInfo(std::string &recv_id_b64) {
	std::lock_guard<std::mutex> lock(m_mutex);
	SignerRpcInfo rpc_info = m_signer_list[recv_id_b64];
	m_signer_cnt++;
	m_mutex.unlock();
	return rpc_info;
  }

private:
  std::unordered_map<string, SignerRpcInfo> m_signer_list;
  std::mutex m_mutex;
  int m_signer_cnt;
};
} // namespace net
} // namespace gruut
