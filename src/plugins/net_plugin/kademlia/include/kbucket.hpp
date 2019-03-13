//        Copyright The Authors 2018.
//    Distributed under the 3-Clause BSD License.
//    (See accompanying file LICENSE or copy at
//   https://opensource.org/licenses/BSD-3-Clause)

//   https://github.com/abdes/blocxxi
#pragma once

#include <chrono>
#include <deque>
#include <utility>  // for std::pair
#include <bitset>

#include <boost/iterator/iterator_adaptor.hpp>
#include <boost/iterator/reverse_iterator.hpp>

#include "../config/network_config.hpp"
#include "node.hpp"

namespace gruut {
namespace net {

class KBucket {
public:

  template <typename TValue, typename TIterator>
  class NodeIterator
	  : public boost::iterator_adaptor<NodeIterator<TValue, TIterator>,
									   TIterator, TValue,
									   std::bidirectional_iterator_tag> {
  public:
	NodeIterator() : NodeIterator::iterator_adaptor_() {}
	explicit NodeIterator(TIterator node)
		: NodeIterator::iterator_adaptor_(node) {}

	template <class OtherValue>
	NodeIterator(NodeIterator<OtherValue, TIterator> const &other,
				 typename std::enable_if<
					 std::is_convertible<OtherValue *, TValue *>::value>::type)
		: NodeIterator::iterator_adaptor_(other.base()) {}
  };

  using iterator = NodeIterator<Node, std::deque<Node>::iterator>;
  using const_iterator =
  NodeIterator<Node const, std::deque<Node>::const_iterator>;
  using reverse_iterator = boost::reverse_iterator<iterator>;
  using const_reverse_iterator = boost::reverse_iterator<const_iterator>;

public:

  KBucket(Node node, unsigned int depth, std::size_t ksize);
  KBucket(const KBucket &) = delete;
  KBucket &operator=(const KBucket &) = delete;
  KBucket(KBucket &&) = default;
  KBucket &operator=(KBucket &&) = default;
  ~KBucket() = default;

  iterator begin() noexcept { return iterator(m_nodes.begin()); };
  const_iterator begin() const noexcept {
	return const_iterator(m_nodes.cbegin());
  }
  iterator end() noexcept { return iterator(m_nodes.end()); }
  const_iterator end() const noexcept { return const_iterator(m_nodes.cend()); }

  reverse_iterator rbegin() noexcept { return reverse_iterator(end()); }
  const_reverse_iterator rbegin() const noexcept {
	return const_reverse_iterator(cend());
  }
  reverse_iterator rend() noexcept { return reverse_iterator(begin()); }
  const_reverse_iterator rend() const noexcept {
	return const_reverse_iterator(cbegin());
  }

  const_iterator cbegin() const noexcept {
	return const_iterator(m_nodes.cbegin());
  }
  const_iterator cend() const noexcept { return const_iterator(m_nodes.cend()); }
  const_reverse_iterator crbegin() const noexcept {
	return const_reverse_iterator(cend());
  }
  const_reverse_iterator crend() const noexcept {
	return const_reverse_iterator(cbegin());
  }

  std::pair<unsigned int, unsigned int> size() const;

  bool empty() const;
  bool full() const;

  unsigned int depth() const;

  std::chrono::seconds timeSinceLastUpdated() const;

  bool canHoldNode(const HashedIdType &node) const;

  std::string sharedPrefix() const {
	return m_prefix.to_string().substr(0, m_prefix_size);
  }
  //@}


  Node &SendConvRequest() { return m_nodes.front(); }

  Node const &leastRecentlySeenNode() const { return m_nodes.front(); }

  Node const &selectRandomNode() const;

  bool addNode(Node &&node);

  void removeNode(const Node &node);

  void removeNode(iterator &node_iter);

  std::pair<KBucket, KBucket> split();

private:

  Node m_my_node;

  std::deque<Node> m_nodes{};

  std::deque<Node> m_replacement_nodes{};

  bool hasReplacements() const;


  unsigned int m_depth{0};
  std::size_t m_ksize;

  std::bitset<KEYSIZE_BITS> m_prefix;

  std::size_t m_prefix_size{0};

  mutable std::chrono::steady_clock::time_point m_last_updated;

  void touchLastUpdated() const;
};

std::ostream &operator<<(std::ostream &out, KBucket const &kb);

}  // namespace net
}  // namespace gruut

