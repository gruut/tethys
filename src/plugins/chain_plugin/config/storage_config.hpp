#ifndef TETHYS_PUBLIC_MERGER_CONFIG_HPP
#define TETHYS_PUBLIC_MERGER_CONFIG_HPP

#include "storage_type.hpp"

#include <chrono>
#include <string>

namespace tethys {
namespace config {

// clang-format off

// APP INFO

        const std::string APP_NAME = "Merger for Tethys Public Networks (C++)";
        const std::string APP_CODE_NAME = "Amaranth Seed";
        const std::string APP_BUILD_DATE = __DATE__;
        const std::string APP_BUILD_TIME = __TIME__;

// SETTING

        constexpr uint32_t MAX_SIGNER_NUM = 200;
        constexpr uint32_t AVAILABLE_INPUT_SIZE = 1000;
        constexpr uint32_t MAX_MERKLE_LEAVES = 4096;
        constexpr uint32_t MAX_COLLECT_TRANSACTION_SIZE = 4096;
        constexpr uint32_t BLOCK_CONFIRM_LEVEL = 3;
        constexpr uint32_t MIN_SIGNATURE_COLLECT_SIZE = 1;
        constexpr uint32_t MAX_SIGNATURE_COLLECT_SIZE = 20;
        constexpr uint32_t MAX_UNICAST_MISSING_BLOCK = 4;
        constexpr uint32_t DB_SESSION_POOL_SIZE = 10;
        constexpr uint32_t BP_INTERVAL = 10;
        constexpr auto BLOCK_POOL_CHECK_PERIOD = std::chrono::milliseconds(500);

// KNOWLEDGE

        const std::string DEFAULT_PORT_NUM = "50051";
        const std::string GENESIS_BLOCK_PREV_HASH_B64 = "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA=";
        const std::string GENESIS_BLOCK_PREV_ID_B58 = "11111111111111111111111111111111";

        const std::string DEFAULT_KV_PATH = "./leveldb";

        static vector<string> keyValueDBNames() {
          vector<string> names;

          names.emplace_back(DataType::BUILT_IN_CONTRACT);
          names.emplace_back(DataType::WORLD);
          names.emplace_back(DataType::CHAIN);
          names.emplace_back(DataType::BACKUP_BLOCK);
          names.emplace_back(DataType::BACKUP_USER_LEDGER);
          names.emplace_back(DataType::BACKUP_CONTRACT_LEDGER);
          names.emplace_back(DataType::BACKUP_USER_ATTRIBUTE);
          names.emplace_back(DataType::BACKUP_USER_CERT);
          names.emplace_back(DataType::BACKUP_CONTRACT);
          names.emplace_back(DataType::UNRESOLVED_BLOCK_IDS_KEY);
          names.emplace_back(DataType::SELF_INFO);

          return names;
        }
}
// clang-format on

} // namespace tethys
#endif
