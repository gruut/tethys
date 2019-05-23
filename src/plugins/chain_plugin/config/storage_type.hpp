#ifndef GRUUT_PUBLIC_MERGER_TYPES_HPP
#define GRUUT_PUBLIC_MERGER_TYPES_HPP

#include <string>
#include <vector>

using namespace std;

namespace gruut {

enum class LedgerType : bool { USERSCOPE = true, CONTRACTSCOPE = false };
struct DataType {
  inline static const string WORLD = "world";
  inline static const string CHAIN = "chain";
  inline static const string BACKUP = "backup";
  inline static const string SELF_INFO = "self_info";
};

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
  alphanumeric_type chain_id;
  alphanumeric_type world_id;
  string chain_created_time;

  // policy
  bool allow_custom_contract;
  bool allow_oracle;
  bool allow_tag;
  bool allow_heavy_contract;

  // creator
  base58_type creator_id;
  std::vector<string> creator_cert;
  string creator_sig;
};

using world_type = struct WorldState {
  // world
  alphanumeric_type world_id;
  string world_created_time;

  // key_currency
  string keyc_name;
  string initial_amount;

  // mining_policy
  bool allow_mining;
  string rule;

  // user_policy
  bool allow_anonymous_user;
  string join_fee;

  LocalChainState local_chain_state;

  // authority
  string authority_id;
  std::vector<string> authority_cert;

  // creator
  base58_type creator_id;
  std::vector<string> creator_cert;
  string creator_sig;
};

using proof_type = struct _proof_type {
  base58_type block_id;
  std::vector<std::pair<bool, std::string>> siblings;
};

using ubp_push_result_type = struct _ubp_push_result_type {
  bool linked;
  bool duplicated;
  block_height_type height;
};

using block_info_type = struct _block_info_type {
  base64_type tx_agg_cbor;
  base58_type block_id;
  int tx_pos; // static_merkle_tree에서의 idx
  base64_type tx_output;

  _block_info_type() = default;
  _block_info_type(base64_type tx_agg_cbor_, base58_type block_id_) : tx_agg_cbor(tx_agg_cbor_), block_id(block_id_) {
    tx_pos = -1;
    tx_output = "";
  }
  _block_info_type(base64_type tx_agg_cbor_, base58_type block_id_, int tx_pos_, base64_type tx_output_)
      : tx_agg_cbor(tx_agg_cbor_), block_id(block_id_), tx_pos(tx_pos_), tx_output(tx_output_) {}
};

using self_info_type = struct SelfInfo {
  string enc_sk;
  string cert;
  string id;
  // TODO : may need more info
};

} // namespace gruut
#endif
