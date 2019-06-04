#ifndef GRUUT_PUBLIC_MERGER_KV_STORE_HPP
#define GRUUT_PUBLIC_MERGER_KV_STORE_HPP

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

  bool saveWorld(world_type &world_info);
  bool saveChain(local_chain_type &chain_info);
  bool saveBackup(UnresolvedBlock &block_info);
  bool saveSelfInfo(self_info_type &self_info);

  string getValueByKey(string what, const string &base_keys);

  std::string readBackup(const std::string &key);
  void flushBackup();
  void clearBackup();
  void delBackup(const std::string &block_id_b64);

  void destroyDB();

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
