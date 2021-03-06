#ifndef TETHYS_PUBLIC_MERGER_TYPES_HPP
#define TETHYS_PUBLIC_MERGER_TYPES_HPP

#include "../../../../lib/tethys-utils/src/bytes_builder.hpp"
#include "../../../../lib/tethys-utils/src/sha256.hpp"
#include <bitset>
#include <map>
#include <string>
#include <vector>

using namespace std;

namespace tethys {

enum class QueryType : int { INSERT = 0, UPDATE = 1, DELETE = 2 };
// enum class WhichType : int {
//  USERLEDGER,
//  CONTRACTLEDGER,
//  USERATTRIBUTE,
//  USERCERT,
//  CONTRACT,
//  USERLEDGER_LIST,
//  CONTRACTLEDGER_LIST,
//  USERATTRIBUTE_LIST,
//  USERCERT_LIST,
//  CONTRACT_LIST
//};

const vector<string> BuiltInContractList{// TODO : add more types
                                         "VALUE-TRANSFER"};

struct DataType {
  inline static const string BUILT_IN_CONTRACT = "built_in_contract";
  inline static const string WORLD = "world";
  inline static const string CHAIN = "chain";
  inline static const string BACKUP_BLOCK = "backup_block";
  inline static const string BACKUP_USER_LEDGER = "backup_user_ledger";
  inline static const string BACKUP_CONTRACT_LEDGER = "backup_contract_ledger";
  inline static const string BACKUP_USER_ATTRIBUTE = "backup_user_attribute";
  inline static const string BACKUP_USER_CERT = "backup_user_cert";
  inline static const string BACKUP_CONTRACT = "backup_contract";
  inline static const string UNRESOLVED_BLOCK_IDS_KEY = "UNRESOLVED_BLOCK_IDS_KEY";
  inline static const string SELF_INFO = "self_info";
};

enum class UniqueCheck : int { NO_VALUE = -1, NOT_UNIQUE = -2 };

constexpr auto TRANSACTION_ID_TYPE_SIZE = 32;
constexpr auto ALPHANUMERIC_ID_TYPE_SIZE = 8;

using string = std::string;
using bytes = std::vector<uint8_t>;
using hash_t = std::vector<uint8_t>;
using timestamp_t = uint64_t;
using block_height_type = uint64_t;

using alphanumeric_type = std::string;
using base58_type = std::string;
using base64_type = std::string;
using contract_id_type = std::string; // 사용자 정의 식별자::사용자 아이디::로컬체인 이름::월드 이름
using txagg_cbor_b64 = std::string;
using path_type = std::bitset<256>;

using local_chain_type = struct LocalChainState {
  // chain
  alphanumeric_type chain_id;
  alphanumeric_type world_id;
  string created_time;

  // policy
  bool allow_custom_contract;
  bool allow_oracle;
  bool allow_tag;
  bool allow_heavy_contract;

  // creator
  base58_type creator_id;
  string creator_pk;
  string creator_sig;

  vector<string> tracker_addresses;
};

using world_type = struct WorldState {
  // world
  alphanumeric_type world_id;
  string created_time;

  // key_currency
  string keyc_name;
  string keyc_initial_amount;

  // mining_policy
  bool allow_mining;
  string mining_rule;

  // user_policy
  bool allow_anonymous_user;
  string join_fee;

  // authority
  string authority_id;
  string authority_pk;

  // creator
  base58_type creator_id;
  string creator_pk;
  string creator_sig;

  string eden_sig;
};

using proof_type = struct ProofType {
  base58_type block_id;
  std::vector<std::pair<bool, std::string>> siblings;
};

using block_push_result_type = struct BlockPushResultType {
  base58_type block_id;
  block_height_type block_height;
  int deq_idx;
  int vec_idx;
  bool linked;
  bool duplicated;

  BlockPushResultType() = default;
};

using block_info_type = struct BlockInfoType {
  base64_type tx_agg_cbor;
  base58_type block_id;
  int tx_pos; // static_merkle_tree에서의 idx
  base64_type tx_output;

  BlockInfoType() = default;
  BlockInfoType(base64_type tx_agg_cbor_, base58_type block_id_) : tx_agg_cbor(tx_agg_cbor_), block_id(block_id_) {
    tx_pos = -1;
    tx_output = "";
  }
  BlockInfoType(base64_type tx_agg_cbor_, base58_type block_id_, int tx_pos_, base64_type tx_output_)
      : tx_agg_cbor(tx_agg_cbor_), block_id(block_id_), tx_pos(tx_pos_), tx_output(tx_output_) {}
};

using block_pool_info_type = struct BlockPoolInfoType {
  base58_type block_id{};
  block_height_type block_height{0};
  int deq_idx{-1};
  int vec_idx{-1};

  BlockPoolInfoType() = default;
};

using result_query_info_type = struct ResultQueryInfoType {
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

  ResultQueryInfoType() = default;
};

using self_info_type = struct SelfInfo {
  string enc_sk;
  string cert;
  string id;
  // TODO : may need more info
};

using user_ledger_type = struct UserLedger {
  string var_name;
  string var_value;
  int var_type;
  base58_type uid;
  timestamp_t up_time;
  block_height_type up_block;
  string tag;
  string pid;
  QueryType query_type;
  bool is_empty{true};

  UserLedger() = default;
  UserLedger(string var_name_, int var_type_, base58_type uid_, string tag_)
      : var_name(var_name_), var_type(var_type_), uid(uid_), tag(tag_) {
    BytesBuilder bytes_builder;
    bytes_builder.append(var_name);
    bytes_builder.appendDec(var_type);
    bytes_builder.appendBase<58>(uid);
    bytes_builder.append(tag);
    pid = TypeConverter::bytesToString(Sha256::hash(bytes_builder.getBytes()));
  }
};

using contract_ledger_type = struct ContractLedger {
  string var_name;
  string var_value;
  int var_type;
  contract_id_type cid;
  timestamp_t up_time;
  block_height_type up_block;
  string var_info;
  string pid;
  QueryType query_type;
  bool is_empty{true};

  ContractLedger() = default;
  ContractLedger(string var_name_, int var_type_, string cid_, string var_info_)
      : var_name(var_name_), var_type(var_type_), cid(cid_), var_info(var_info_) {
    BytesBuilder bytes_builder;
    bytes_builder.append(var_name);
    bytes_builder.appendDec(var_type);
    bytes_builder.append(cid);
    bytes_builder.append(var_info);
    pid = TypeConverter::bytesToString(Sha256::hash(bytes_builder.getBytes()));
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
  QueryType query_type;
};

// using which_type = struct WhichData {
//  user_ledger_type user_ledger;
//  contract_ledger_type contract_ledger;
//  user_attribute_type user_attribute;
//  user_cert_type user_cert;
//  contract_type contract;
//
//  map<string, user_ledger_type> user_ledger_list;
//  map<string, contract_ledger_type> contract_ledger_list;
//  WhichType which_type;
//
//  WhichData() = default;
//};

} // namespace tethys
#endif
