#ifndef GRUUT_PUBLIC_MERGER_TYPES_HPP
#define GRUUT_PUBLIC_MERGER_TYPES_HPP

#include <bytes_builder.hpp>
#include <sha256.hpp>
#include <string>
#include <vector>

using namespace std;

namespace tethys {

enum class QueryType : int { INSERT, UPDATE, DELETE };
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
using contract_id_type = std::string; // 사용자 정의 식별자::사용자 아이디::로컬체인 이름::월드 이름
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
  vector<string> creator_cert;
  string creator_sig;

  vector<string> tracker_addresses;
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

  // authority
  string authority_id;
  std::vector<string> authority_cert;

  // creator
  base58_type creator_id;
  std::vector<string> creator_cert;
  string creator_sig;

  string eden_sig;
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

using result_query_info_type = struct _result_query_info_type {
  base58_type block_id;
  block_height_type block_height;
  base58_type tx_id;
  bool status;
  string info;
  base58_type author;
  base58_type user;
  base58_type receiver;
  contract_id_type self;
  std::vector<contract_id_type> friends;
  int fee_author;
  int fee_user;

  _result_query_info_type() = default;
};

using self_info_type = struct SelfInfo {
  string enc_sk;
  string cert;
  string id;
  // TODO : may need more info
};

using user_ledger_type = struct UserLedger {
  string var_name;
  string var_val;
  int var_type;
  base58_type uid;
  timestamp_t up_time;
  block_height_type up_block;
  string tag;
  hash_t pid;
  QueryType query_type;
  bool is_empty{false};

  UserLedger() = default;
  UserLedger(string var_name_, int var_type_, string uid_, string tag_) : var_name(var_name_), var_type(var_type_), uid(uid_), tag(tag_) {
    BytesBuilder bytes_builder;
    bytes_builder.append(var_name);
    bytes_builder.append(var_type);
    bytes_builder.appendBase<58>(uid);
    bytes_builder.append(tag);
    pid = Sha256::hash(bytes_builder.getBytes());
  }
};

using contract_ledger_type = struct ContractLedger {
  string var_name;
  string var_val;
  int var_type;
  string cid;
  timestamp_t up_time;
  block_height_type up_block;
  string var_info;
  hash_t pid;
  QueryType query_type;
  bool is_empty{false};

  ContractLedger() = default;
  ContractLedger(string var_name_, int var_type_, string cid_, string var_info_)
      : var_name(var_name_), var_type(var_type_), cid(cid_), var_info(var_info_) {
    BytesBuilder bytes_builder;
    bytes_builder.append(var_name);
    bytes_builder.append(var_type);
    bytes_builder.append(cid);
    bytes_builder.append(var_info);
    pid = Sha256::hash(bytes_builder.getBytes());
  }
};

using search_result_type = struct LedgerSearchResult {
  bool not_found{false};
  user_ledger_type user_ledger;
  contract_ledger_type contract_ledger;
};

using user_attribute_type = struct UserAttribute {
  base58_type uid;
  timestamp_t register_day;
  string register_code;
  int gender;
  string isc_type;
  string isc_code;
  string location;
  int age_limit;
  string sigma;
};

using user_cert_type = struct UserCert {
  base58_type uid;
  string sn;
  timestamp_t nvbefore;
  timestamp_t nvafter;
  string x509;
};

using contract_type = struct Contract {
  contract_id_type cid;
  timestamp_t after;
  timestamp_t before;
  base58_type author;
  contract_id_type friends;
  string contract;
  string desc;
  string sigma;
};

} // namespace tethys
#endif
