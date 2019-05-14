#include "include/kv_store.hpp"

namespace gruut {

KvController::KvController() {
  logger::INFO("KV store initialize");

  m_options.block_cache = leveldb::NewLRUCache(100 * 1048576); // 100MB cache
  m_options.create_if_missing = true;
  m_write_options.sync = true;

  m_db_path = config::DEFAULT_KV_PATH;
  boost::filesystem::create_directories(m_db_path);
  errorOnCritical(leveldb::DB::Open(m_options, m_db_path + "/" + config::KV_SUB_DIR_WORLD, &m_kv_world));
  errorOnCritical(leveldb::DB::Open(m_options, m_db_path + "/" + config::KV_SUB_DIR_CHAIN, &m_kv_chain));
  errorOnCritical(leveldb::DB::Open(m_options, m_db_path + "/" + config::KV_SUB_DIR_BACKUP, &m_kv_backup));
}

KvController::~KvController() {
  delete m_kv_world;
  delete m_kv_chain;
  delete m_kv_backup;

  m_kv_world = nullptr;
  m_kv_chain = nullptr;
  m_kv_backup = nullptr;
}

bool KvController::errorOnCritical(const leveldb::Status &status) {
  if (status.ok())
    return true;
  else {
    logger::ERROR("KV: FATAL ERROR on LevelDB {}", status.ToString());
    return false;
  }
}

bool KvController::saveWorld(world_type &world_info) {
  // TODO: 리팩토링 필요
  alphanumeric_type tmp_wid = world_info.world_id;
  addBatch(DataType::WORLD, tmp_wid + "_wId", tmp_wid);

  string tmp_ctime = world_info.world_created_time;
  addBatch(DataType::WORLD, tmp_wid + "_ctime", tmp_ctime);

  base58_type tmp_cid = world_info.creator_id;
  addBatch(DataType::WORLD, tmp_wid + "_cid", tmp_cid);

  string tmp_cpk = parseCertContent(world_info.creator_cert);
  addBatch(DataType::WORLD, tmp_wid + "_cpk", tmp_cpk);

  base58_type tmp_aid = world_info.authority_id;
  addBatch(DataType::WORLD, tmp_wid + "_aid", tmp_aid);

  string tmp_apk = parseCertContent(world_info.authority_cert);
  addBatch(DataType::WORLD, tmp_wid + "_apk", tmp_apk);

  string tmp_kcn = world_info.keyc_name;
  addBatch(DataType::WORLD, tmp_wid + "_kcn", tmp_kcn);

  string tmp_kcia = world_info.initial_amount;
  addBatch(DataType::WORLD, tmp_wid + "_kcia", tmp_kcia);

  bool tmp_amine = world_info.allow_mining;
  addBatch(DataType::WORLD, tmp_wid + "_amine", to_string(tmp_amine));

  string tmp_mrule = world_info.rule;
  addBatch(DataType::WORLD, tmp_wid + "_mrule", tmp_mrule);

  bool tmp_aauser = world_info.allow_anonymous_user;
  addBatch(DataType::WORLD, tmp_wid + "_tmp_aauser", to_string(tmp_aauser));

  string tmp_jfee = world_info.join_fee;
  addBatch(DataType::WORLD, tmp_wid + "_tmp_jfee", tmp_jfee);

  commitBatchAll();

  return true;
}

bool KvController::saveChain(local_chain_type &chain_info) {
  // TODO: World는 unmarshalGenesisState가 다 해주지만, chain은 추가로 더 선언될 수 있으므로 파싱 추가구현 필요.
  // TODO: 리팩토링 필요

  alphanumeric_type tmp_chid = chain_info.chain_id;
  addBatch(DataType::CHAIN, tmp_chid + "_chid", tmp_chid);

  string tmp_ctime = chain_info.chain_created_time;
  addBatch(DataType::CHAIN, tmp_chid + "_ctime", tmp_ctime);

  base58_type tmp_crid = chain_info.creator_id;
  addBatch(DataType::CHAIN, tmp_chid + "_crid", tmp_crid);

  string tmp_cpk = parseCertContent(chain_info.creator_cert);
  addBatch(DataType::CHAIN, tmp_chid + "_cpk", tmp_cpk);

  bool tmp_acc = chain_info.allow_custom_contract;
  addBatch(DataType::CHAIN, tmp_chid + "_acc", to_string(tmp_acc));

  bool tmp_ao = chain_info.allow_oracle;
  addBatch(DataType::CHAIN, tmp_chid + "_ao", to_string(tmp_ao));

  bool tmp_atag = chain_info.allow_tag;
  addBatch(DataType::CHAIN, tmp_chid + "_atag", to_string(tmp_atag));

  bool tmp_ahc = chain_info.allow_heavy_contract;
  addBatch(DataType::CHAIN, tmp_chid + "_ahc", to_string(tmp_ahc));

  commitBatchAll();

  return true;
}

bool KvController::saveBackup() {

  commitBatchAll();

  return true;
}

bool KvController::addBatch(DataType what, const string &base_key, const string &value) {
  string key = base_key;
  switch (what) {
  case DataType::WORLD:
    m_batch_world.Put(key, value);
    break;
  case DataType::CHAIN:
    m_batch_chain.Put(key, value);
    break;
  case DataType::BACKUP:
    m_batch_backup.Put(key, value);
    break;
  default:
    break;
  }
  return true;
}

void KvController::commitBatchAll() {
  m_kv_world->Write(m_write_options, &m_batch_world);
  m_kv_chain->Write(m_write_options, &m_batch_chain);
  m_kv_backup->Write(m_write_options, &m_batch_backup);

  clearBatchAll();
}

void KvController::rollbackBatchAll() {
  clearBatchAll();
}

void KvController::clearBatchAll() {
  m_batch_world.Clear();
  m_batch_chain.Clear();
  m_batch_backup.Clear();
}

string KvController::getValueByKey(DataType what, const string &base_keys) {
  std::string key = base_keys;
  std::string value;
  leveldb::Status status;

  switch (what) {
  case DataType::WORLD:
    status = m_kv_world->Get(m_read_options, key, &value);
    break;
  case DataType::CHAIN:
    status = m_kv_chain->Get(m_read_options, key, &value);
    break;
  case DataType::BACKUP:
    status = m_kv_backup->Get(m_read_options, key, &value);
    break;
  default:
    break;
  }

  if (!status.ok())
    value = "";
  return value;
}

void KvController::destroyDB() {
  boost::filesystem::remove_all(m_db_path + "/" + config::KV_SUB_DIR_WORLD);
  boost::filesystem::remove_all(m_db_path + "/" + config::KV_SUB_DIR_CHAIN);
  boost::filesystem::remove_all(m_db_path + "/" + config::KV_SUB_DIR_BACKUP);
}

string KvController::parseCertContent(std::vector<string> &cert) {
  string cert_content = "";
  for (int i = 1; i < cert.size() - 1; ++i) {
    cert_content += cert[i];
  }
  return cert_content;
}

}