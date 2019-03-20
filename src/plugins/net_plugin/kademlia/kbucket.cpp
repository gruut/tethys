//        Copyright The Authors 2018.
//    Distributed under the 3-Clause BSD License.
//    (See accompanying file LICENSE or copy at
//   https://opensource.org/licenses/BSD-3-Clause)

//   https://github.com/abdes/blocxxi

#include <random>

#include <boost/multiprecision/cpp_int.hpp>
#include <vector>
#include "include/kbucket.hpp"

namespace gruut {
  namespace net_plugin {

    using namespace boost::multiprecision;

    KBucket::KBucket(Node node, unsigned int depth, std::size_t ksize)
            : m_my_node(std::move(node)), m_depth(depth), m_ksize(ksize) {
      touchLastUpdated();
    }

    bool KBucket::addNode(Node &&node) {

      auto found = std::find_if(
              m_nodes.begin(), m_nodes.end(),
              [&node](Node &bucket_node) { return (bucket_node == node); });
      if (found != m_nodes.end()) {
        m_nodes.erase(found);

        node.openChannel();
        m_nodes.emplace_back(std::move(node));
      } else if (!full()) {
        node.openChannel();
        m_nodes.emplace_back(std::move(node));
      } else {

        m_replacement_nodes.emplace_back(std::move(node));
        return false;
      }
      touchLastUpdated();
      return true;
    }

    void KBucket::removeNode(Node const &node) {
      touchLastUpdated();

      auto node_found =
              std::find_if(begin(), end(), [&node](Node const &bucket_node) {
                return (bucket_node.getIdHash() == node.getIdHash());
              });
      if (node_found != end()) {
        removeNode(node_found);
      } else {

        auto replacement_found =
                std::find_if(m_replacement_nodes.begin(), m_replacement_nodes.end(),
                             [&node](Node const &replacement_node) {
                               return (replacement_node.getIdHash() == node.getIdHash());
                             });
        if (replacement_found != m_replacement_nodes.end()) {
          m_replacement_nodes.erase(replacement_found);
        }
      }
    }

    void KBucket::removeNode(KBucket::iterator &node_iter) {
      touchLastUpdated();

      m_nodes.erase(node_iter.base());

      if (!m_replacement_nodes.empty()) {

        m_replacement_nodes.back().openChannel();
        m_nodes.emplace_back(std::move(m_replacement_nodes.back()));
        m_replacement_nodes.pop_back();
      }
    }

    bool KBucket::canHoldNode(const HashedIdType &node) const {

      auto id_bits = node.toBitSet();
      auto last_bit = KEYSIZE_BITS - m_prefix_size;
      for (auto bit = KEYSIZE_BITS - 1; bit >= last_bit; --bit) {
        if (m_prefix[bit] ^ id_bits[bit]) return false;
      }
      return true;
    }

    std::pair<KBucket, KBucket> KBucket::split() {

      auto my_id = m_my_node.getIdHash().toBitSet();
      auto one = KBucket(m_my_node, m_depth + 1, m_ksize);
      one.m_prefix = m_prefix;
      auto two = KBucket(m_my_node, m_depth + 1, m_ksize);
      two.m_prefix = m_prefix;
      if (my_id.test(KEYSIZE_BITS - 1 - m_prefix_size)) {
        one.m_prefix.reset(KEYSIZE_BITS - 1 - m_prefix_size);
        two.m_prefix.set(KEYSIZE_BITS - 1 - m_prefix_size);
      } else {
        one.m_prefix.set(KEYSIZE_BITS - 1 - m_prefix_size);
        two.m_prefix.reset(KEYSIZE_BITS - 1 - m_prefix_size);
      }
      one.m_prefix_size = m_prefix_size + 1;
      two.m_prefix_size = m_prefix_size + 1;

      for (Node &node : m_nodes) {
        KBucket &bucket = one.canHoldNode(node.getIdHash()) ? one : two;
        bucket.m_nodes.push_back(std::move(node));
      }
      m_nodes.clear();

      if (hasReplacements()) {

        for (Node &node : m_replacement_nodes) {
          KBucket &bucket = one.canHoldNode(node.getIdHash()) ? one : two;
          bucket.m_replacement_nodes.push_back(std::move(node));
        }
        m_replacement_nodes.clear();
      }

      return std::make_pair<KBucket, KBucket>(std::move(one), std::move(two));
    }

    struct NodeDistanceComparator {
      const Node &ref_node_;

      explicit NodeDistanceComparator(const Node &node) : ref_node_(node) {}

      bool operator()(const Node &lhs, const Node &rhs) const {
        return ref_node_.distanceTo(lhs) < ref_node_.distanceTo(rhs);
      }
    };

    std::vector<Node> KBucket::selectAliveNodes() {
      std::vector<Node> alive_nodes = {};
      std::copy_if(m_nodes.begin(), m_nodes.end(), std::back_inserter(alive_nodes), [](auto &n) {
        return n.isAlive();
      });

      return alive_nodes;
    }

    void KBucket::removeDeadNodes() {
      m_nodes.erase(std::remove_if(m_nodes.begin(), m_nodes.end(),[](auto &n){ return !n.isAlive(); }),
          m_nodes.end());
    }

    bool KBucket::empty() const { return m_nodes.empty(); }

    bool KBucket::full() const { return m_nodes.size() == m_ksize; }

    std::pair<unsigned int, unsigned int> KBucket::size() const {
      return std::make_pair(m_nodes.size(), m_replacement_nodes.size());
    }

    std::chrono::seconds KBucket::timeSinceLastUpdated() const {
      return std::chrono::duration_cast<std::chrono::seconds>(
              std::chrono::steady_clock::now() - m_last_updated);
    }

    bool KBucket::hasReplacements() const { return !m_replacement_nodes.empty(); }

    void KBucket::touchLastUpdated() const {
      m_last_updated = std::chrono::steady_clock::now();
    }

    unsigned int KBucket::depth() const { return m_depth; }

    std::ostream &operator<<(std::ostream &out, KBucket const &kb) {
      out << "entries:" << kb.size().first << " replacements:" << kb.size().second;
      return out;
    }
  }  // namespace net_plugin
}  // namespace gruut
