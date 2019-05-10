#ifndef GRUUT_PUBLIC_MERGER_KV_STORE_HPP
#define GRUUT_PUBLIC_MERGER_KV_STORE_HPP

#include "../../../../lib/json/include/json.hpp"
#include "../config/storage_type.hpp"

#include <boost/filesystem/operations.hpp>
#include <leveldb/cache.h>
#include <leveldb/db.h>
#include <leveldb/options.h>
#include <leveldb/write_batch.h>

#include <cmath>
#include <deque>
#include <iostream>
#include <map>

using namespace std;
typedef unsigned int uint;

namespace gruut {

class KVStore {
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
  KVStore();
  ~KVStore();

  bool empty();

  void saveBackup(const std::string &key, const std::string &value);
  std::string readBackup(const std::string &key);
  void flushBackup();
  void clearBackup();
  void delBackup(const std::string &block_id_b64);

private:
  bool errorOnCritical(const leveldb::Status &status);
  void readConfig();
  void destroyDB();
};
} // namespace gruut

#endif
