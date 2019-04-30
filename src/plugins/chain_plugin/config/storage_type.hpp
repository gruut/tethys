#ifndef GRUUT_PUBLIC_MERGER_TYPES_HPP
#define GRUUT_PUBLIC_MERGER_TYPES_HPP

#include <string>
#include <vector>

namespace gruut {

enum class LedgerType : bool { USERSCOPE = true, CONTRACTSCOPE = false };

// DB
enum class DBType : int { BLOCK_HEADER, BLOCK_HEIGHT, BLOCK_RAW, BLOCK_LATEST, TRANSACTION, LEDGER, BLOCK_BACKUP };

using string = std::string;
using bytes = std::vector<uint8_t>;
using hash_t = std::vector<uint8_t>;
using timestamp_t = uint64_t;

constexpr auto TRANSACTION_ID_TYPE_SIZE = 32;
constexpr auto ALPHANUMERIC_ID_TYPE_SIZE = 8;

using block_height_type = size_t;
using alphanumeric_type = std::string;
using base58_type = std::string;
using base64_type = std::string;
using txagg_cbor_b64 = std::string;


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
