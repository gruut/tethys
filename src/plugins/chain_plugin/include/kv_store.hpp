#ifndef GRUUT_PUBLIC_MERGER_KV_STORE_HPP
#define GRUUT_PUBLIC_MERGER_KV_STORE_HPP

#include "../../../../lib/json/include/json.hpp"
#include "../../../../lib/log/include/log.hpp"
#include "../config/storage_config.hpp"
#include "../config/storage_type.hpp"

#include <boost/filesystem/operations.hpp>
#include <leveldb/cache.h>
#include <leveldb/db.h>
#include <leveldb/options.h>
#include <leveldb/write_batch.h>

#include <iostream>

using namespace std;
typedef unsigned int uint;

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

  void saveBackup(const std::string &key, const std::string &value);
  std::string readBackup(const std::string &key);
  void flushBackup();
  void clearBackup();
  void delBackup(const std::string &block_id_b64);

private:
  bool errorOnCritical(const leveldb::Status &status);
  void destroyDB();
};
} // namespace gruut

#endif
