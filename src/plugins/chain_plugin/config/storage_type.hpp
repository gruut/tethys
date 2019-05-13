#ifndef GRUUT_PUBLIC_MERGER_TYPES_HPP
#define GRUUT_PUBLIC_MERGER_TYPES_HPP

#include <string>
#include <vector>

namespace gruut {

enum class LedgerType : bool { USERSCOPE = true, CONTRACTSCOPE = false };
enum class DBType : int { BLOCK_HEADER, BLOCK_HEIGHT, BLOCK_RAW, BLOCK_LATEST, TRANSACTION, LEDGER, BLOCK_BACKUP };


using string = std::string;
using bytes = std::vector<uint8_t>;
using hash_t = std::vector<uint8_t>;
using timestamp_t = uint64_t;

constexpr auto TRANSACTION_ID_TYPE_SIZE = 32;
constexpr auto ALPHANUMERIC_ID_TYPE_SIZE = 8;

using block_height_type = int32_t;
using alphanumeric_type = std::string;
using base58_type = std::string;
using base64_type = std::string;
using txagg_cbor_b64 = std::string;


using local_chain_type = struct LocalChainState {
  // chain
  string chain_id;
  string world_id;
  string chain_created_time;

  // policy
  bool allow_custom_contract;
  bool allow_oracle;
  bool allow_tag;
  bool allow_heavy_contract;

  // creator
  string creator_id;
  std::vector<string> creator_cert;
  string creator_sig;
};

using world_type = struct WorldState {
  // world
  string world_id;
  string world_created_time;

  // key_currency
  string keyc_name;
  string initial_amount;

  // mining_policy
  bool allow_mining;
  void *rule = nullptr;

  // user_policy
  bool allow_anonymous_user;
  string join_fee;

  LocalChainState local_chain_state;

  // authority
  string authority_id;
  std::vector<string> authority_cert;

  // creator
  string creator_id;
  std::vector<string> creator_cert;
  string creator_sig;
};

using proof_type = struct _proof_type {
  std::string block_id_b58;
  std::vector<std::pair<bool, std::string>> siblings;
};

using unblk_push_result_type = struct _unblk_push_result_type {
  bool linked;
  bool duplicated;
  block_height_type height;
};

} // namespace gruut
#endif
