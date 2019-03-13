//        Copyright The Authors 2018.
//    Distributed under the 3-Clause BSD License.
//    (See accompanying file LICENSE or copy at
//   https://opensource.org/licenses/BSD-3-Clause)

//   https://github.com/abdes/blocxxi

#pragma once

#include <array>
#include <bitset>
#include <cstdint>
#include <cstring>
#include <iosfwd>
#include <iterator>
#include <string>
#include <vector>
#include <algorithm>
#include <memory>

#include <botan-2/botan/base64.h>
#include <botan-2/botan/hash.h>

 namespace gruut{
 namespace net{
template <unsigned int BITS>
class Hash {
  static_assert(BITS % 32 == 0, "Hash size in bits must be a multiple of 32");
  static_assert(BITS > 0, "Hash size in bits must be greater than 0");

public:
  using value_type = std::uint8_t;
  using pointer = uint8_t *;
  using const_pointer = std::uint8_t const *;
  using size_type = std::size_t;
  using difference_type = std::ptrdiff_t;
  using reference = std::uint8_t &;
  using const_reference = std::uint8_t const &;
  using iterator = pointer;
  using const_iterator = const_pointer;
  using reverse_iterator = std::reverse_iterator<iterator>;
  using const_reverse_iterator = std::reverse_iterator<const_iterator>;

  Hash() : m_storage({}) {}

  Hash(const Hash &other) = default;

  Hash &operator=(const Hash &rhs) = default;

  Hash(Hash &&) noexcept = default;

  Hash &operator=(Hash &&rhs) noexcept = default;

  virtual ~Hash() = default;

  static Hash max() noexcept {
	Hash h;
	h.m_storage.fill(0xffffffffU);
	return h;
  }

  static Hash min() noexcept {
	return Hash();
	// all bits are already 0
  }


  static Hash<160> sha1(const std::string &src) {
	Hash<160> hash;

	std::unique_ptr<Botan::HashFunction> hash_function(
		Botan::HashFunction::create("SHA-1"));

	hash_function->update(src);
	auto stdvec = hash_function->final_stdvec();

	std::copy_n(stdvec.begin(), SIZE_BYTE, hash.m_storage.begin());

	return hash;
  }

  reference at(size_type pos) {
	if (size() <= pos) {
	  throw std::out_of_range("Hash<BITS> index out of range");
	}
	return reinterpret_cast<pointer>(m_storage.data())[pos];
  }
  const_reference at(size_type pos) const {
	if (size() <= pos) {
	  throw std::out_of_range("Hash<BITS> index out of range");
	}
	return reinterpret_cast<const_pointer>(m_storage.data())[pos];
  };

  reference operator[](size_type pos) noexcept {
	//ASAP_ASSERT_PRECOND(pos < size());
	return reinterpret_cast<pointer>(m_storage.data())[pos];
  }
  const_reference operator[](size_type pos) const noexcept {
	//ASAP_ASSERT_PRECOND(pos < size());
	return reinterpret_cast<pointer>(m_storage.data())[pos];
  }

  reference front() { return reinterpret_cast<pointer>(m_storage.data())[0]; }
  const_reference front() const {
	return reinterpret_cast<const_pointer>(m_storage.data())[0];
  }

  reference back() {
	return reinterpret_cast<pointer>(m_storage.data())[size() - 1];
  }
  const_reference back() const {
	return reinterpret_cast<const_pointer>(m_storage.data())[size() - 1];
  }

  pointer data() noexcept { return reinterpret_cast<pointer>(m_storage.data()); }
  const_pointer data() const noexcept {
	return reinterpret_cast<const_pointer>(m_storage.data());
  }

  iterator begin() noexcept {
	return reinterpret_cast<pointer>(m_storage.data());
  }
  const_iterator begin() const noexcept {
	return reinterpret_cast<const_pointer>(m_storage.data());
  }
  const_iterator cbegin() const noexcept { return begin(); }

  iterator end() noexcept {
	return reinterpret_cast<pointer>(m_storage.data()) + size();
  }
  const_iterator end() const noexcept {
	return reinterpret_cast<const_pointer>(m_storage.data()) + size();
  }
  const_iterator cend() const noexcept { return end(); }

  reverse_iterator rbegin() noexcept { return reverse_iterator(end()); }
  const_reverse_iterator rbegin() const noexcept {
	return const_reverse_iterator(end());
  }
  const_reverse_iterator crbegin() const noexcept { return rbegin(); }

  reverse_iterator rend() noexcept { return reverse_iterator(begin()); }
  const_reverse_iterator rend() const noexcept {
	return const_reverse_iterator(begin());
  }
  const_reverse_iterator crend() const noexcept { return rend(); }

  constexpr static std::size_t size() { return BITS / 8; };

  constexpr static std::size_t bitSize() { return BITS; };

  void clear() { m_storage.fill(0); }

  bool isAllZero() const {
	for (auto v : m_storage)
	  if (v != 0) return false;
	return true;
  }

  void swap(Hash &other) noexcept { m_storage.swap(other.m_storage); }

  friend bool operator==(const Hash<BITS> &lhs, const Hash<BITS> &rhs) {
	return std::equal(lhs.begin(), lhs.end(), rhs.begin());
  }

  friend bool operator<(const Hash<BITS> &lhs, const Hash<BITS> &rhs) {
    for(std::size_t i = 0; i < SIZE_BYTE; ++i){
      if(lhs.m_storage[i] < rhs.m_storage[i]) return true;
      if(lhs.m_storage[i] > lhs.m_storage[i]) return false;
    }
    return false;
  }

  Hash &operator^=(Hash const &other) noexcept {
	for (std::size_t i = 0; i < SIZE_BYTE; ++i)
	  m_storage[i] ^= other.m_storage[i];
	return *this;
  }

  std::bitset<BITS> toBitSet() const {
	std::bitset<BITS> bs;  // [0,0,...,0]
	int shift_left = BITS;
	for (uint8_t num_part : m_storage) {
	  shift_left -= 8;
	  std::bitset<BITS> bs_part(num_part);
	  bs |= bs_part << shift_left;
	}
	return bs;
  }

  std::string toBitStringShort(std::size_t length = 32U) const {
	auto truncate = false;
	if (length < BITS) { truncate = true; length -= 3; }
	auto str = toBitSet().to_string().substr(0, length);
	if (truncate) str.append("...");
	return str;
  }

private:

  static constexpr std::size_t SIZE_BYTE = BITS / 8;

  std::array<std::uint8_t , SIZE_BYTE> m_storage;
};

template <unsigned int BITS>
inline bool operator!=(const Hash<BITS> &lhs, const Hash<BITS> &rhs) {
  return !(lhs == rhs);
}
template <unsigned int BITS>
inline bool operator>(const Hash<BITS> &lhs, const Hash<BITS> &rhs) {
  return rhs < lhs;
}
template <unsigned int BITS>
inline bool operator<=(const Hash<BITS> &lhs, const Hash<BITS> &rhs) {
  return !(lhs > rhs);
}
template <unsigned int BITS>
inline bool operator>=(const Hash<BITS> &lhs, const Hash<BITS> &rhs) {
  return !(lhs < rhs);
}
//@}

template <unsigned int BITS>
inline Hash<BITS> operator^(Hash<BITS> lhs, Hash<BITS> const &rhs) noexcept {
  return lhs.operator^=(rhs);
}

template <unsigned int BITS>
inline void swap(Hash<BITS> &lhs, Hash<BITS> &rhs) {
  lhs.swap(rhs);
}

template <unsigned int BITS>
inline std::ostream &operator<<(std::ostream &out, Hash<BITS> const &hash) {
  out << hash.ToHex();
  return out;
}

using Hash512 = Hash<512>;  // 64 bytes
using Hash256 = Hash<256>;  // 32 bytes
using Hash160 = Hash<160>;  // 20 bytes

 } //namespace net
 } //namespace gruut