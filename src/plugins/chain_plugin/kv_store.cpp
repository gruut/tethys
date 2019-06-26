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

bool KvController::saveBuiltInContracts(map<string, string> &contracts) {
  for (auto &contract : contracts) {
    addBatch(DataType::BUILT_IN_CONTRACT, contract.first, contract.second);
  }

  commitBatchAll();

  return true;
}

bool KvController::saveLatestWorldId(const alphanumeric_type &world_id) {
  addBatch(DataType::WORLD, "latest_world_id", world_id);

  commitBatchAll();

  return true;
}

bool KvController::saveLatestChainId(const alphanumeric_type &chain_id) {
  addBatch(DataType::CHAIN, "latest_chain_id", chain_id);

  commitBatchAll();

  return true;
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

  string tk_addr_list;
  string delimiter = ",";
  for (auto &tracker_addr : chain_info.tracker_addresses)
    tk_addr_list += (tracker_addr + delimiter);
  tk_addr_list.pop_back();
  addBatch(DataType::CHAIN, tmp_chid + "_tk_addr", tk_addr_list);

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

bool KvController::saveBlockIds(const string &serialized_block_ids) {
  addBatch(DataType::UNRESOLVED_BLOCK_IDS_KEY, DataType::UNRESOLVED_BLOCK_IDS_KEY, serialized_block_ids);

  commitBatchAll();
  return true;
}

bool KvController::saveBackupBlock(const base58_type &block_id, const string &serialized_block) {
  addBatch(DataType::BACKUP_BLOCK, block_id, serialized_block);

  commitBatchAll();
  return true;
}

bool KvController::saveBackupUserLedgers(const base58_type &block_id, const string &serialized_user_ledgers) {
  addBatch(DataType::BACKUP_USER_LEDGER, block_id, serialized_user_ledgers);

  commitBatchAll();
  return true;
}

bool KvController::saveBackupContractLedgers(const base58_type &block_id, const string &serialized_contract_ledgers) {
  addBatch(DataType::BACKUP_CONTRACT_LEDGER, block_id, serialized_contract_ledgers);

  commitBatchAll();
  return true;
}

bool KvController::saveBackupUserAttributes(const base58_type &block_id, const string &serialized_user_attributes) {
  addBatch(DataType::BACKUP_USER_ATTRIBUTE, block_id, serialized_user_attributes);

  commitBatchAll();
  return true;
}

bool KvController::saveBackupUserCerts(const base58_type &block_id, const string &serialized_user_certs) {
  addBatch(DataType::BACKUP_USER_CERT, block_id, serialized_user_certs);

  commitBatchAll();
  return true;
}

bool KvController::saveBackupContracts(const base58_type &block_id, const string &serialized_contracts) {
  addBatch(DataType::BACKUP_CONTRACT, block_id, serialized_contracts);

  commitBatchAll();
  return true;
}

string KvController::loadBlockIds() {
  return getValueByKey(DataType::UNRESOLVED_BLOCK_IDS_KEY, DataType::UNRESOLVED_BLOCK_IDS_KEY);
}

string KvController::loadBackupBlock(const std::string &key) {
  return getValueByKey(DataType::BACKUP_BLOCK, key);
}

string KvController::loadBackupUserLedgers(const std::string &key) {
  return getValueByKey(DataType::BACKUP_USER_LEDGER, key);
}

string KvController::loadBackupContractLedgers(const std::string &key) {
  return getValueByKey(DataType::BACKUP_CONTRACT_LEDGER, key);
}

string KvController::loadBackupUserAttributes(const std::string &key) {
  return getValueByKey(DataType::BACKUP_USER_ATTRIBUTE, key);
}

string KvController::loadBackupUserCerts(const std::string &key) {
  return getValueByKey(DataType::BACKUP_USER_CERT, key);
}

string KvController::loadBackupContracts(const std::string &key) {
  return getValueByKey(DataType::BACKUP_CONTRACT, key);
}

void KvController::delBackup(const base58_type &block_id) {
  if (!block_id.empty()) {
    write_batch_map[DataType::BACKUP_BLOCK].Delete(block_id);
    write_batch_map[DataType::BACKUP_USER_LEDGER].Delete(block_id);
    write_batch_map[DataType::BACKUP_CONTRACT_LEDGER].Delete(block_id);
    write_batch_map[DataType::BACKUP_USER_ATTRIBUTE].Delete(block_id);
    write_batch_map[DataType::BACKUP_USER_CERT].Delete(block_id);
    write_batch_map[DataType::BACKUP_CONTRACT].Delete(block_id);

    commitBatchAll();
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