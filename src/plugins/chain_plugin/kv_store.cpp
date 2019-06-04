#include "include/kv_store.hpp"

namespace tethys {

KvController::KvController() {
  logger::INFO("KV store initialize");

  m_options.block_cache = leveldb::NewLRUCache(100 * 1048576); // 100MB cache
  m_options.create_if_missing = true;
  m_write_options.sync = true;

  m_db_path = config::DEFAULT_KV_PATH;
  boost::filesystem::create_directories(m_db_path);

  auto dir_names = config::keyValueDBNames();
  for (auto &dir_name : dir_names) {
    leveldb::DB *db;
    errorOnCritical(leveldb::DB::Open(m_options, m_db_path + "/" + dir_name, &db));
    db_map[dir_name] = db;

    write_batch_map[dir_name] = leveldb::WriteBatch();
  }
}

KvController::~KvController() {
  for (auto &[_, db_ptr] : db_map) {
    delete db_ptr;
    db_ptr = nullptr;
  }
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

bool KvController::saveBackup(UnresolvedBlock &block_info) {

  commitBatchAll();

  return true;
}

bool KvController::saveSelfInfo(self_info_type &self_info) {
  addBatch(DataType::SELF_INFO, "self_enc_sk", self_info.enc_sk);
  addBatch(DataType::SELF_INFO, "self_cert", self_info.cert);
  addBatch(DataType::SELF_INFO, "self_id", self_info.id);

  commitBatchAll();

  return true;
}

bool KvController::addBatch(string what, const string &key, const string &value) {
  write_batch_map[what].Put(key, value);
  return true;
}

void KvController::commitBatchAll() {
  for (auto &[name, db_ptr] : db_map) {
    db_ptr->Write(m_write_options, &write_batch_map[name]);
  }

  clearBatchAll();
}

void KvController::rollbackBatchAll() {
  clearBatchAll();
}

void KvController::clearBatchAll() {
  for (auto &[_, write_batch] : write_batch_map) {
    write_batch.Clear();
  }
}

string KvController::getValueByKey(string what, const string &key) {
  string value;
  leveldb::Status status;

  status = db_map[what]->Get(m_read_options, key, &value);

  if (!status.ok())
    value = "";
  return value;
}

void KvController::destroyDB() {
  for (auto &[db_name, _] : db_map) {
    boost::filesystem::remove_all(m_db_path + "/" + db_name);
  }
}

string KvController::parseCertContent(std::vector<string> &cert) {
  string cert_content = "";
  for (int i = 0; i < cert.size(); ++i) {
    cert_content += cert[i];
    cert_content += "\n";
  }
  return cert_content;
}

}