#ifndef TETHYS_PUBLIC_MERGER_KV_STORE_HPP
#define TETHYS_PUBLIC_MERGER_KV_STORE_HPP

#include "../../../../lib/json/include/json.hpp"
#include "../../../../lib/log/include/log.hpp"
#include "../config/storage_config.hpp"
#include "../config/storage_type.hpp"
#include "unresolved_block_pool.hpp"

#include <boost/filesystem/operations.hpp>
#include <leveldb/cache.h>
#include <leveldb/db.h>
#include <leveldb/options.h>
#include <leveldb/write_batch.h>

#include <iostream>
#include <unordered_map>

using namespace std;

namespace tethys {

class KvController {
private:
  string m_db_path;

  leveldb::Options m_options;
  leveldb::WriteOptions m_write_options;
  leveldb::ReadOptions m_read_options;

  unordered_map<string, leveldb::DB *> db_map;
  unordered_map<string, leveldb::WriteBatch> write_batch_map;

public:
  KvController();
  ~KvController();

  bool saveBuiltInContracts(map<string, string> &contracts);
  bool saveLatestWorldId(const alphanumeric_type &world_id);
  bool saveLatestChainId(const alphanumeric_type &chain_id);
  bool saveWorld(world_type &world_info);
  bool saveChain(local_chain_type &chain_info);
  bool saveSelfInfo(self_info_type &self_info);

  string getValueByKey(string what, const string &base_keys);
  void destroyDB();

  // unresolved block pool backup / restore
  bool saveBlockIds(const string &serialized_block_ids);
  bool saveBackupBlock(const base58_type &block_id, const string &serialized_block);
  bool saveBackupUserLedgers(const base58_type &block_id, const string &serialized_user_ledgers);
  bool saveBackupContractLedgers(const base58_type &block_id, const string &serialized_contract_ledgers);
  bool saveBackupUserAttributes(const base58_type &block_id, const string &serialized_user_attributes);
  bool saveBackupUserCerts(const base58_type &block_id, const string &serialized_user_certs);
  bool saveBackupContracts(const base58_type &block_id, const string &serialized_contracts);

  string loadBlockIds();
  string loadBackupBlock(const std::string &key);
  string loadBackupUserLedgers(const std::string &key);
  string loadBackupContractLedgers(const std::string &key);
  string loadBackupUserAttributes(const std::string &key);
  string loadBackupUserCerts(const std::string &key);
  string loadBackupContracts(const std::string &key);

  void delBackup(const base58_type &block_id);

private:
  bool errorOnCritical(const leveldb::Status &status);
  bool addBatch(string what, const string &key, const string &value);
  void commitBatchAll();
  void rollbackBatchAll();
  void clearBatchAll();

  string parseCertContent(std::vector<string> &cert);
};
} // namespace tethys
#endif
