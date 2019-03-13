//        Copyright The Authors 2018.
//    Distributed under the 3-Clause BSD License.
//    (See accompanying file LICENSE or copy at
//   https://opensource.org/licenses/BSD-3-Clause)

//   https://github.com/abdes/blocxxi

#pragma once

#include <chrono>
#include <vector>
#include <deque>
#include <forward_list>
#include <set>
#include <utility>
#include <unordered_map>
#include <mutex>

#include <boost/iterator/iterator_adaptor.hpp>
#include <boost/iterator/reverse_iterator.hpp>

#include "kbucket.hpp"
#include "../config/network_config.hpp"

namespace gruut {
namespace net {

class RoutingTable  {
public:

  template <typename TValue, typename TIterator>
  class BucketIterator
	  : public boost::iterator_adaptor<BucketIterator<TValue, TIterator>,
									   TIterator, TValue,
									   std::bidirectional_iterator_tag> {
  public:
	BucketIterator() : BucketIterator::iterator_adaptor_() {}
	explicit BucketIterator(TIterator node)
		: BucketIterator::iterator_adaptor_(node) {}

	template <class OtherValue>
	BucketIterator(
		BucketIterator<OtherValue, TIterator> const &other,
		typename std::enable_if<
			std::is_convertible<OtherValue *, TValue *>::value>::type)
		: BucketIterator::iterator_adaptor_(other.base()) {}
  };

  using iterator = BucketIterator<KBucket, std::deque<KBucket>::iterator>;
  using const_iterator =
  BucketIterator<KBucket const, std::deque<KBucket>::const_iterator>;
  using reverse_iterator = boost::reverse_iterator<iterator>;
  using const_reverse_iterator = boost::reverse_iterator<const_iterator>;

public:

  RoutingTable(Node node, std::size_t ksize);


  RoutingTable(const RoutingTable &) = delete;

  RoutingTable &operator=(const RoutingTable &) = delete;

  RoutingTable(RoutingTable &&) = default;

  RoutingTable &operator=(RoutingTable &&) = default;

  ~RoutingTable() = default;

  iterator begin() noexcept { return iterator(m_buckets.begin()); };
  const_iterator begin() const noexcept {
	return const_iterator(m_buckets.cbegin());
  }
  iterator end() noexcept { return iterator(m_buckets.end()); }
  const_iterator end() const noexcept {
	return const_iterator(m_buckets.cend());
  }

  reverse_iterator rbegin() noexcept { return reverse_iterator(end()); }
  const_reverse_iterator rbegin() const noexcept {
	return const_reverse_iterator(cend());
  }
  reverse_iterator rend() noexcept { return reverse_iterator(begin()); }
  const_reverse_iterator rend() const noexcept {
	return const_reverse_iterator(cbegin());
  }

  const_iterator cbegin() const noexcept {
	return const_iterator(m_buckets.cbegin());
  }
  const_iterator cend() const noexcept {
	return const_iterator(m_buckets.cend());
  }
  const_reverse_iterator crbegin() const noexcept {
	return const_reverse_iterator(cend());
  }
  const_reverse_iterator crend() const noexcept {
	return const_reverse_iterator(cbegin());
  }
  //@}


  const Node &thisNode() const { return m_my_node; }


  std::size_t nodesCount() const;

  std::size_t bucketsCount() const;

  bool empty() const;

  bool addPeer(Node &&peer);


  void removePeer(const Node &peer);

  bool peerTimedOut(Node const &peer);

  std::vector<Node> findNeighbors(HashedIdType const &id)  {
	return findNeighbors(id, m_ksize);
  };

  std::vector<Node> findNeighbors(HashedIdType const &id,
								  std::size_t max_number);

  size_t getBucketIndexFor(const HashedIdType &node) const;

private:

  void addInitialBucket() { m_buckets.push_front(KBucket(m_my_node, 0, m_ksize)); }

  Node m_my_node;

  std::size_t m_ksize;

  std::deque<KBucket> m_buckets;

  std::unordered_map<IdType, IpEndpoint> m_node_table;

  std::mutex m_buckets_mutex;
};

std::ostream &operator<<(std::ostream &out, RoutingTable const &rt);

}  // namespace net
}  // namespace gruut
