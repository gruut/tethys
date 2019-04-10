//        Copyright The Authors 2018.
//    Distributed under the 3-Clause BSD License.
//    (See accompanying file LICENSE or copy at
//   https://opensource.org/licenses/BSD-3-Clause)

//   https://github.com/abdes/blocxxi

#include <sstream> // for ToString() implementation

#include <boost/multiprecision/cpp_int.hpp>

#include "include/routing.hpp"

namespace gruut {
namespace net_plugin {

using boost::multiprecision::cpp_int_backend;
using boost::multiprecision::number;
using boost::multiprecision::unchecked;
using boost::multiprecision::unsigned_magnitude;

constexpr unsigned int DEPTH_B = 5;

RoutingTable::RoutingTable(Node node, std::size_t ksize) : m_my_node(std::move(node)), m_ksize(ksize) {
  m_buckets.emplace_front(KBucket(m_my_node, 0, m_ksize));
}

std::size_t RoutingTable::nodesCount() const {
  std::size_t total = 0;
  for (auto &kb : m_buckets)
    total += kb.size().first;
  return total;
}

std::size_t RoutingTable::bucketsCount() const {
  return m_buckets.size();
}

bool RoutingTable::empty() const {
  return m_buckets.front().empty();
}

std::size_t RoutingTable::getBucketIndexFor(const hashed_net_id_type &node) const {

  auto num_buckets = m_buckets.size();

  auto bucket = m_buckets.begin();
  while (bucket != m_buckets.end()) {
    if (bucket->canHoldNode(node)) {
      auto bucket_index = static_cast<std::size_t>(std::distance(m_buckets.begin(), bucket));

      return bucket_index;
    }
    ++bucket;
  }

  return num_buckets - 1;
}

bool RoutingTable::addPeer(Node &&peer) {
  if (m_my_node == peer) {
    return true;
  }

  peer.initReqFailuresCount();

  std::lock_guard<std::mutex> lock(m_buckets_mutex);

  auto bucket_index = getBucketIndexFor(peer.getIdHash());
  auto bucket = m_buckets.begin();
  std::advance(bucket, bucket_index);

  bool check = bucket->addNode(std::move(peer));

  m_buckets_mutex.unlock();

  if (check) {
    return true;
  }

  auto can_split = false;

  auto shared_prefix_test = (bucket->depth() < DEPTH_B) && ((bucket->depth() % DEPTH_B) != 0);

  auto bucket_has_in_range_my_node = (bucket_index == (m_buckets.size() - 1));
  can_split |= bucket_has_in_range_my_node;

  can_split |= shared_prefix_test;

  can_split &= (m_buckets.size() < 160);

  can_split &= !(m_buckets.size() > 1 && bucket_index == 0);

  if (can_split) {
    std::pair<KBucket, KBucket> pair = bucket->split();
    bucket = m_buckets.insert(bucket, std::move(pair.second));
    bucket = m_buckets.insert(bucket, std::move(pair.first));

    std::lock_guard<std::mutex> lock(m_buckets_mutex);
    m_buckets.erase(bucket + 2);

    m_buckets_mutex.unlock();

    return true;
  }

  return false;
}

void RoutingTable::removePeer(const Node &peer) {

  std::lock_guard<std::mutex> lock(m_buckets_mutex);
  auto bucket_index = getBucketIndexFor(peer.getIdHash());
  auto bucket = m_buckets.begin();
  std::advance(bucket, bucket_index);

  bucket->removeNode(peer);
  m_buckets_mutex.unlock();
}

bool RoutingTable::peerTimedOut(Node const &peer) {
  for (auto bucket = m_buckets.rbegin(); bucket != m_buckets.rend(); ++bucket) {
    for (auto bn = bucket->begin(); bn != bucket->end(); ++bn) {
      if (bn->getIdHash() == peer.getIdHash()) {
        if (bn->isStale()) {
          std::lock_guard<std::mutex> lock(m_buckets_mutex);
          bucket->removeNode(bn);
          m_buckets_mutex.unlock();
          return true;
        } else {
          bn->incReqFailuresCount();
          return false;
        }
      }
    }
  }

  return false;
}

std::optional<Node> RoutingTable::findNode(const hashed_net_id_type &hashed_id) {
  auto bucket_index = getBucketIndexFor(hashed_id);
  auto bucket = m_buckets.begin();
  std::advance(bucket, bucket_index);

  for (auto &node : *bucket) {
    if (hashed_id == node.getIdHash()) {
      return node;
    }
  }
  return {};
}

std::optional<Node> RoutingTable::findNode(const b58_user_id_type &id) {
  if (m_id_mapping_table.count(id) > 0) {
    return findNode(m_id_mapping_table[id]);
  }
  return {};
}

std::vector<Node> RoutingTable::findNeighbors(hashed_net_id_type const &id, std::size_t max_number) {
  std::vector<Node> neighbors;
  auto count = 0U;

  bool use_left = true;
  bool has_more = true;

  std::lock_guard<std::mutex> lock(m_buckets_mutex);

  auto bucket_index = getBucketIndexFor(id);
  auto bucket = m_buckets.begin();
  std::advance(bucket, bucket_index);
  auto left = bucket;
  auto right = (bucket != m_buckets.end()) ? bucket + 1 : bucket;

  auto current_bucket = bucket;

  while (has_more) {
    has_more = false;
    for (auto const &neighbor : *current_bucket) {
      // Exclude the node
      if (neighbor.getIdHash() != id) {
        ++count;

        neighbors.emplace_back(neighbor);
        if (count == max_number)
          goto Done;
      }
    }

    if (right == m_buckets.end())
      use_left = true;
    if (left != m_buckets.begin()) {
      has_more = true;
      if (use_left) {
        --left;
        current_bucket = left;
        use_left = false;
        continue;
      }
    }
    if (right != m_buckets.end()) {
      has_more = true;
      current_bucket = right;
      ++right;
    }
    use_left = true;
  }
Done:

  m_buckets_mutex.unlock();

  return neighbors;
}

void RoutingTable::mapId(const b58_user_id_type &b58_user_id, const net_id_type &net_id) {
  std::lock_guard<std::mutex> guard(m_id_map_mutex);
  m_id_mapping_table.try_emplace(b58_user_id, Hash<160>::sha1(net_id));
  m_id_map_mutex.unlock();
}
void RoutingTable::unmapId(const b58_user_id_type &b58_user_id) {
  if (m_id_mapping_table.count(b58_user_id) > 0) {
    std::lock_guard<std::mutex> guard(m_id_map_mutex);
    m_id_mapping_table.erase(b58_user_id);
    m_id_map_mutex.unlock();
  }
}
void RoutingTable::unmapId(const hashed_net_id_type &dead_hashed_id) {
  for (auto &[b58_user_id, net_hased_id] : m_id_mapping_table) {
    if (net_hased_id == dead_hashed_id) {
      unmapId(b58_user_id);
      return;
    }
  }
}

} // namespace net_plugin
} // namespace gruut
