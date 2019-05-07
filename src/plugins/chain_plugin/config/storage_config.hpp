#ifndef GRUUT_PUBLIC_MERGER_CONFIG_HPP
#define GRUUT_PUBLIC_MERGER_CONFIG_HPP

#include "storage_type.hpp"

#include <string>

namespace gruut {
namespace config {

// clang-format off

// APP INFO

        const std::string APP_NAME = "Merger for Gruut Public Networks (C++)";
        const std::string APP_CODE_NAME = "???";
        const std::string APP_BUILD_DATE = __DATE__;
        const std::string APP_BUILD_TIME = __TIME__;

// SETTING

        constexpr int32_t MAX_SIGNER_NUM = 200;
        constexpr int32_t AVAILABLE_INPUT_SIZE = 1000;
        constexpr int32_t MAX_MERKLE_LEAVES = 4096;
        constexpr int32_t MAX_COLLECT_TRANSACTION_SIZE = 4096;
        constexpr int32_t BLOCK_CONFIRM_LEVEL = 3;
        constexpr int32_t MIN_SIGNATURE_COLLECT_SIZE = 1;
        constexpr int32_t MAX_SIGNATURE_COLLECT_SIZE = 20;
        constexpr int32_t MAX_UNICAST_MISSING_BLOCK = 4;
        constexpr int32_t DB_SESSION_POOL_SIZE = 10;
        constexpr int32_t BP_INTERVAL = 10;

// KNOWLEDGE

        const std::string DEFAULT_PORT_NUM = "50051";
        const std::string DEFAULT_DB_PATH = "./db";
        const std::string GENESIS_BLOCK_PREV_HASH_B64 = "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA=";
        const std::string GENESIS_BLOCK_PREV_ID_B64 = "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA=";
        const std::string DB_SUB_DIR_HEADER = "block_header";
        const std::string DB_SUB_DIR_RAW = "block_raw";
        const std::string DB_SUB_DIR_LATEST = "latest_block_header";
        const std::string DB_SUB_DIR_TRANSACTION = "transaction";
        const std::string DB_SUB_DIR_IDHEIGHT = "blockid_height";
        const std::string DB_SUB_DIR_LEDGER = "ledger";
        const std::string DB_SUB_DIR_BACKUP = "backup";
        const std::string DB_SUB_DIR_MERGER_INFO = "merger_info";

// clang-format on

} // namespace config
} // namespace gruut
#endif
