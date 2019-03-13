//        Copyright The Authors 2018.
//    Distributed under the 3-Clause BSD License.
//    (See accompanying file LICENSE or copy at
//   https://opensource.org/licenses/BSD-3-Clause)

//   https://github.com/abdes/blocxxi
#pragma once

#include <chrono>
#include <string>

#include "../config/network_config.hpp"
#include "../config/network_type.hpp"

namespace gruut {
namespace net {

class Node {
public:
  Node() = default;

  Node(HashedIdType const &id_hash, IdType const &id, std::string const &ip_address,
	   std::string const &port_number)
	  : m_id_hash{id_hash}, m_id{id}, m_endpoint{ip_address, port_number} {}

  Node(HashedIdType const &id_hash, IdType const &id, IpEndpoint const &ep) : m_id_hash(id_hash), m_id(id), m_endpoint(ep) {}

  Node(const Node &other) : m_id_hash(other.m_id_hash),
  							m_id(other.m_id),
							m_endpoint(other.m_endpoint),
							m_failed_requests_count(other.m_failed_requests_count),
							m_last_seen_time(other.m_last_seen_time) {}

  Node &operator=(Node const &rhs) {
	using std::swap;
	auto tmp(rhs);
	swap(*this, tmp);
	return *this;
  }

  Node(Node &&other) noexcept : m_id_hash(std::move(other.m_id_hash)),
  								m_id(std::move(other.m_id)),
								m_endpoint(std::move(other.m_endpoint)),
								m_failed_requests_count(other.m_failed_requests_count),
								m_last_seen_time(other.m_last_seen_time) {
  }

  Node &operator=(Node &&rhs) noexcept {
	if (this != &rhs) {
	  m_id_hash = std::move(rhs.m_id_hash);
	  m_endpoint = std::move(rhs.m_endpoint);
	  m_failed_requests_count = rhs.m_failed_requests_count;
	  m_last_seen_time = rhs.m_last_seen_time;
	}
	return *this;
  }

  ~Node() = default;

  friend bool operator==(const Node &lhs, const Node &rhs) {
	return (lhs.getIdHash() == rhs.getIdHash()) && (lhs.getEndpoint() == rhs.getEndpoint());
  }

  HashedIdType const &getIdHash() const { return m_id_hash; }
  IdType const &getId() const { return m_id; }
  IpEndpoint const &getEndpoint() const { return m_endpoint; }

  int failuresCount() const { return m_failed_requests_count; }
  void initFailuresCount() { m_failed_requests_count = 0; }

  bool isStale() const {
	return m_failed_requests_count == NODE_FAILED_COMMS_BEFORE_STALE;
  }

  bool isQuestionable() const {
	return (std::chrono::steady_clock::now() - m_last_seen_time)
		> NODE_INACTIVE_TIME_BEFORE_QUESTIONABLE;
  }

  HashedIdType distanceTo(Node const &node) const { return distanceTo(node.getIdHash()); }
  HashedIdType distanceTo(Hash160 const &hash) const {
	return m_id_hash ^ hash;
  }

  void incFailuresCount() { ++m_failed_requests_count; }

  HashedIdType &getIdHash() { return m_id_hash; }
  IdType &getId()  { return m_id; }
  IpEndpoint &getEndpoint() { return m_endpoint; }

private:
  IdType  m_id;
  HashedIdType m_id_hash;
  IpEndpoint m_endpoint;

  int m_failed_requests_count{0};

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


inline bool operator!=(const Node &lhs, const Node &rhs) {
  return !(lhs == rhs);
}


}  // namespace net
}  // namespace gruut
