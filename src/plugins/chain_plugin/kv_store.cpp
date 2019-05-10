#include "include/kv_store.hpp"

namespace gruut {

KVStore::KVStore() {
  logger::ERROR("KV");

  m_options.block_cache = leveldb::NewLRUCache(100 * 1048576); // 100MB cache
  m_options.create_if_missing = true;
  m_write_options.sync = true;

  m_db_path = config::DEFAULT_KV_PATH;
  boost::filesystem::create_directories(m_db_path);
  errorOnCritical(leveldb::DB::Open(m_options, m_db_path + "/" + config::KV_SUB_DIR_WORLD, &m_kv_world));
  errorOnCritical(leveldb::DB::Open(m_options, m_db_path + "/" + config::KV_SUB_DIR_CHAIN, &m_kv_chain));
  errorOnCritical(leveldb::DB::Open(m_options, m_db_path + "/" + config::KV_SUB_DIR_BACKUP, &m_kv_backup));
}

KVStore::~KVStore() {
  delete m_kv_world;
  delete m_kv_chain;
  delete m_kv_backup;

  m_kv_world = nullptr;
  m_kv_chain = nullptr;
  m_kv_backup = nullptr;
}

bool KVStore::errorOnCritical(const leveldb::Status &status) {
  if (status.ok())
    return true;

  logger::ERROR("KV: ATAL ERROR on LevelDB {}", status.ToString());

  return false;
}

void KVStore::destroyDB() {
  boost::filesystem::remove_all(m_db_path + "/" + config::KV_SUB_DIR_WORLD);
  boost::filesystem::remove_all(m_db_path + "/" + config::KV_SUB_DIR_CHAIN);
  boost::filesystem::remove_all(m_db_path + "/" + config::KV_SUB_DIR_BACKUP);
}

}