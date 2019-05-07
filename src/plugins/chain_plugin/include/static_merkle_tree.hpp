#ifndef GRUUT_PUBLIC_MERGER_MERKLE_TREE_HPP
#define GRUUT_PUBLIC_MERGER_MERKLE_TREE_HPP

#include <map>
#include <vector>

#include "../../../../lib/gruut-utils/src/bytes_builder.hpp"
#include "../../../../lib/gruut-utils/src/sha256.hpp"
#include "../../../../lib/gruut-utils/src/type_converter.hpp"
#include "../config/storage_config.hpp"
#include "../config/storage_type.hpp"
#include "../structure/transaction.hpp"

using namespace std;
using namespace gruut::config;

namespace gruut {

// clang-format off
const std::map<std::string, std::string> HASH_LOOKUP_B64 = {
    {"AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA==", "9aX9QtFqIDAnmO9u0wmXm0MAPSMg2fDo6pgxqSdZ+0s="}, // 1
    {"9aX9QtFqIDAnmO9u0wmXm0MAPSMg2fDo6pgxqSdZ+0v1pf1C0WogMCeY727TCZebQwA9IyDZ8OjqmDGpJ1n7Sw==", "21YRTgD91MH4XIkr81rJqJKJquyx69CpbN5ganSLXXE="}, // 2
    {"21YRTgD91MH4XIkr81rJqJKJquyx69CpbN5ganSLXXHbVhFOAP3UwfhciSvzWsmokomq7LHr0Kls3mBqdItdcQ==", "x4AJ/fB/xWoR8SI3BlijU6qlQu1j5ExLwV/0zRBaszw="}, // 3
    {"x4AJ/fB/xWoR8SI3BlijU6qlQu1j5ExLwV/0zRBaszzHgAn98H/FahHxIjcGWKNTqqVC7WPkTEvBX/TNEFqzPA==", "U22Yg38t0WWlXV7q6RSFlURy1W8kbfJWvzyuGTUqEjw="}, // 4
    {"U22Yg38t0WWlXV7q6RSFlURy1W8kbfJWvzyuGTUqEjxTbZiDfy3RZaVdXurpFIWVRHLVbyRt8la/PK4ZNSoSPA==", "nv3gUqoVQp+uBbrU0LHXxk2mTQPXoYVKWIwsuEMMDTA="}, // 5
    {"nv3gUqoVQp+uBbrU0LHXxk2mTQPXoYVKWIwsuEMMDTCe/eBSqhVCn64FutTQsdfGTaZNA9ehhUpYjCy4QwwNMA==", "2I3f7tQAqHVVlrIZQsFJfhFMMC5hGCkPkeZ3KXYEH6E="}, // 6
    {"2I3f7tQAqHVVlrIZQsFJfhFMMC5hGCkPkeZ3KXYEH6HYjd/u1ACodVWWshlCwUl+EUwwLmEYKQ+R5ncpdgQfoQ==", "h+sN26V+NfbShmc4AqSvWXXiJQbHz0xku2vl7hFSfyw="}, // 7
    {"h+sN26V+NfbShmc4AqSvWXXiJQbHz0xku2vl7hFSfyyH6w3bpX419tKGZzgCpK9ZdeIlBsfPTGS7a+XuEVJ/LA==", "JoRkdv1fxUpdQzhRZ8lRRPJkP1M8yFu50Wt4L419sZM="}, // 8
    {"JoRkdv1fxUpdQzhRZ8lRRPJkP1M8yFu50Wt4L419sZMmhGR2/V/FSl1DOFFnyVFE8mQ/UzzIW7nRa3gvjX2xkw==", "UG2GWC0lJAW4QAGHksrSvxJZ8e9apfiH4Tyy8AlPUeE="}, // 9
    {"UG2GWC0lJAW4QAGHksrSvxJZ8e9apfiH4Tyy8AlPUeFQbYZYLSUkBbhAAYeSytK/Elnx71ql+IfhPLLwCU9R4Q==", "//8K1+ZZdy+VNMGVyBXvxAFO8eHa7UQEwGOF0RGS6Ss="}, // 10
    {"//8K1+ZZdy+VNMGVyBXvxAFO8eHa7UQEwGOF0RGS6Sv//wrX5ll3L5U0wZXIFe/EAU7x4drtRATAY4XREZLpKw==", "bPBBJ9sFRBzYMxB6Ur6FKGiJDkMX5qAqtHaDqnWWQiA="} // 11
};

static std::map<bytes, hash_t> HASH_LOOKUP = {};
// clang-format on

class StaticMerkleTree {
private:
  vector<hash_t> m_merkle_tree;

public:
  StaticMerkleTree() {
    prepareLookUpTable();
  }

  StaticMerkleTree(vector<hash_t> &merkle_contents) {
    prepareLookUpTable();
    generate(merkle_contents);
  }

  void generate(vector<hash_t> &merkle_contents) {
    const bytes dummy_leaf(32, 0); // for SHA-256

    auto min_addable_size = min(MAX_MERKLE_LEAVES, merkle_contents.size());

    for (int i = 0; i < min_addable_size; ++i)
      m_merkle_tree[i] = merkle_contents[i];

    for (int i = min_addable_size; i < MAX_MERKLE_LEAVES; ++i)
      m_merkle_tree[i] = dummy_leaf;

    int parent_pos = MAX_MERKLE_LEAVES;
    for (int i = 0; i < MAX_MERKLE_LEAVES * 2 - 3; i += 2) {
      m_merkle_tree[parent_pos] = makeParent(m_merkle_tree[i], m_merkle_tree[i + 1]);
      ++parent_pos;
    }
  }

  vector<hash_t> getStaticMerkleTree() {
    return m_merkle_tree;
  }

  static bool isValidSiblings(proof_type &proof,
                              const std::string &root_val_b64) { // 필요한 함수인지 검토 필요
    return isValidSiblings(proof.siblings, proof.siblings[0].second, root_val_b64);
  }

  static bool isValidSiblings(std::vector<std::pair<bool, std::string>> &siblings_b64, const std::string &my_val_b64,
                              const std::string &root_val_b64) {

    if (siblings_b64.empty() || my_val_b64.empty() || root_val_b64.empty())
      return false;

    if (siblings_b64[0].second != my_val_b64)
      return false;

    std::vector<std::pair<bool, bytes>> siblings;
    for (auto &sibling_b64 : siblings_b64) {
      siblings.emplace_back(
          std::make_pair(sibling_b64.first, TypeConverter::stringToBytes(TypeConverter::decodeBase<64>(sibling_b64.second))));
    }

    bytes my_val = TypeConverter::stringToBytes(TypeConverter::decodeBase<64>(my_val_b64));
    bytes root_val = TypeConverter::stringToBytes(TypeConverter::decodeBase<64>(root_val_b64));

    return isValidSiblings(siblings, my_val, root_val);
  }

  static bool isValidSiblings(std::vector<std::pair<bool, bytes>> &siblings, bytes &my_val, bytes &root_val) {

    if (siblings.empty() || my_val.empty() || root_val.empty())
      return false;

    if (siblings[0].second != my_val)
      return false;

    hash_t mtree_root;
    for (int i = 0; i < siblings.size(); ++i) {
      if (i == 0) {
        mtree_root = static_cast<hash_t>(siblings[i].second);
        continue;
      }

      if (siblings[i].first) { // true = right
        mtree_root.insert(mtree_root.end(), siblings[i].second.begin(), siblings[i].second.end());
      } else {
        mtree_root.insert(mtree_root.begin(), siblings[i].second.begin(), siblings[i].second.end());
      }

      mtree_root = Sha256::hash(mtree_root);
    }

    return (root_val == mtree_root);
  }

private:
  void prepareLookUpTable() {
    m_merkle_tree.resize(MAX_MERKLE_LEAVES * 2 - 1);

    if (HASH_LOOKUP.size() != HASH_LOOKUP_B64.size()) {
      HASH_LOOKUP.clear();
      for (auto &hash_entry : HASH_LOOKUP_B64) {
        HASH_LOOKUP.emplace(TypeConverter::stringToBytes(TypeConverter::decodeBase<64>(hash_entry.first)),
                            TypeConverter::stringToBytes(TypeConverter::decodeBase<64>(hash_entry.second)));
      }
    }
  }

  hash_t makeParent(hash_t left, hash_t &right) {
    left.insert(left.cend(), right.cbegin(), right.cend());

    auto it_map = HASH_LOOKUP.find(left);
    if (it_map != HASH_LOOKUP.end()) {
      return it_map->second;
    } else {
      return Sha256::hash(left);
    }
  }
};
} // namespace gruut

#endif