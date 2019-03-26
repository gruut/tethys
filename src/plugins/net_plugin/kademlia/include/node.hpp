//        Copyright The Authors 2018.
//    Distributed under the 3-Clause BSD License.
//    (See accompanying file LICENSE or copy at
//   https://opensource.org/licenses/BSD-3-Clause)

//   https://github.com/abdes/blocxxi
#pragma once

#include <chrono>
#include <grpc/impl/codegen/connectivity_state.h>
#include <grpcpp/channel.h>
#include <grpcpp/grpcpp.h>
#include <memory>
#include <string>

#include "../../config/include/network_config.hpp"
#include "../../config/include/network_type.hpp"
#include "hash.hpp"

namespace gruut {
namespace net_plugin {

constexpr int MAX_TRY_CONN_COUNT = 3;
constexpr auto NODE_INACTIVE_TIME_BEFORE_QUESTIONABLE = std::chrono::minutes(15);
constexpr unsigned int NODE_FAILED_COMMS_BEFORE_STALE = 2;

class Node {
public:
  Node(HashedIdType const &id_hash, IdType const &id, std::string const &ip_address, std::string const &port_number)
      : m_id_hash{id_hash}, m_id{id}, m_endpoint{ip_address, port_number} {}

  Node(HashedIdType const &id_hash, IdType const &id, std::string const &ip_address, std::string const &port_number,
       std::shared_ptr<grpc::Channel> c)
      : m_id_hash{id_hash}, m_id{id}, m_endpoint{ip_address, port_number}, m_channel_ptr(c) {}

  friend bool operator==(Node &lhs, Node &rhs) {
    return (lhs.getIdHash() == rhs.getIdHash()) && (lhs.getEndpoint() == rhs.getEndpoint());
  }

  HashedIdType const &getIdHash() const {
    return m_id_hash;
  }

  IdType const &getId() const {
    return m_id;
  }

  IpEndpoint const &getEndpoint() const {
    return m_endpoint;
  }

  bool isStale() const {
    return m_failed_requests_count == NODE_FAILED_COMMS_BEFORE_STALE;
  }

  bool isQuestionable() const {
    return (std::chrono::steady_clock::now() - m_last_seen_time) > NODE_INACTIVE_TIME_BEFORE_QUESTIONABLE;
  }

  bool isAlive() {
    if (m_channel_ptr == nullptr || failuresTryConnCount() > MAX_TRY_CONN_COUNT)
      return false;

    auto channel_stat = m_channel_ptr->GetState(true);
    if (channel_stat == GRPC_CHANNEL_READY) {
      resetTryConnCount();
      return true;
    } else if (channel_stat == GRPC_CHANNEL_CONNECTING) {
      incTryConnCount();
    }
    return false;
  }

  HashedIdType distanceTo(Node const &node) const {
    return distanceTo(node.getIdHash());
  }

  HashedIdType distanceTo(Hash160 const &hash) const {
    return m_id_hash ^ hash;
  }

  void openChannel() {
    if (m_channel_ptr != nullptr)
      return;
    auto credential = grpc::InsecureChannelCredentials();
    m_channel_ptr = grpc::CreateChannel(m_endpoint.address + ":" + m_endpoint.port, credential);
    m_channel_ptr->GetState(true);
  }

  int failuresReqCount() const {
    return m_failed_requests_count;
  }

  void initReqFailuresCount() {
    m_failed_requests_count = 0;
  }

  void incReqFailuresCount() {
    ++m_failed_requests_count;
  }

  int failuresTryConnCount() const {
    return m_failed_try_conn_count;
  }

  void resetTryConnCount() {
    m_failed_try_conn_count = 0;
  }

  void incTryConnCount() {
    ++m_failed_try_conn_count;
  }

  std::shared_ptr<grpc::Channel> getChannelPtr() {
    return m_channel_ptr;
  }

  HashedIdType &getIdHash() {
    return m_id_hash;
  }

  IdType &getId() {
    return m_id;
  }

  IpEndpoint &getEndpoint() {
    return m_endpoint;
  }

private:
  IdType m_id;
  HashedIdType m_id_hash;
  IpEndpoint m_endpoint;
  std::shared_ptr<grpc::Channel> m_channel_ptr;

  int m_failed_requests_count{0};
  int m_failed_try_conn_count{0};

  std::chrono::steady_clock::time_point m_last_seen_time;
};

inline HashedIdType distance(Node const &a, Node const &b) {
  return a.distanceTo(b);
}

inline HashedIdType distance(Node const &node, Hash160 const &hash) {
  return node.distanceTo(hash);
}

inline HashedIdType distance(HashedIdType const &ida, HashedIdType const &idb) {
  return ida ^ idb;
}

inline bool operator!=(Node &lhs, Node &rhs) {
  return !(lhs == rhs);
}
} // namespace net_plugin
} // namespace gruut
