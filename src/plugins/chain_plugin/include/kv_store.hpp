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

using namespace std;

namespace gruut {

class KvController {
private:
  string m_db_path;

  leveldb::Options m_options;
  leveldb::WriteOptions m_write_options;
  leveldb::ReadOptions m_read_options;

  leveldb::DB *m_kv_world;
  leveldb::DB *m_kv_chain;
  leveldb::DB *m_kv_backup;

  leveldb::WriteBatch m_batch_world;
  leveldb::WriteBatch m_batch_chain;
  leveldb::WriteBatch m_batch_backup;

public:
  KvController();
  ~KvController();

  bool saveWorld(world_type &world_info);
  bool saveChain(local_chain_type &chain_info);
  bool saveBackup(UnresolvedBlock &block_info);

  string getValueByKey(DataType what, const string &base_keys);

  std::string readBackup(const std::string &key);
  void flushBackup();
  void clearBackup();
  void delBackup(const std::string &block_id_b64);

  void destroyDB();

private:
  bool errorOnCritical(const leveldb::Status &status);
  bool addBatch(DataType what, const string &base_key, const string &value);
  void commitBatchAll();
  void rollbackBatchAll();
  void clearBatchAll();

  string parseCertContent(std::vector<string> &cert);
};
} // namespace gruut
#endif
