#include "include/rdb_controller.hpp"

using namespace std;

namespace gruut {

RdbController::RdbController(string_view dbms, string_view table_name, string_view db_user_id, string_view db_password)
    : m_dbms(dbms), m_table_name(table_name), m_db_user_id(db_user_id), m_db_password(db_password),
      m_db_pool(config::DB_SESSION_POOL_SIZE) {
  for (int i = 0; i != config::DB_SESSION_POOL_SIZE; ++i) {
    soci::session &sql = m_db_pool.at(i);
    sql.open(m_dbms, "service=" + m_table_name + " user=" + m_db_user_id + " password=" + m_db_password);
  }
  logger::INFO("DB pool initialize");
}

soci::connection_pool &RdbController::pool() {
  return m_db_pool;
}

bool RdbController::insertBlockData(Block &block) {
  logger::INFO("insert Block Data");

  soci::row result;
  soci::session db_session(RdbController::pool());

  // clang-format off
  string block_id = block.getBlockId();
  string block_hash = block.getBlockHash();
  string prev_block_id = block.getPrevBlockId();
  string pre_block_sig = block.getPrevBlockSig();
  string block_prod_id = block.getBlockProdId();
  string block_pro_sig = block.getBlockProdSig();
  string tx_id = block.getTransactions()[0].getTxID();
  string tx_root = block.getTxRoot();
  string user_state_root = block.getUserStateRoot();
  string contract_state_root = block.getContractStateRoot();
  string sig_root = block.getSgRoot();
  string aggz = block.getAggz();
  string block_cert = block.getBlockCert();

  soci::statement st = (db_session.prepare << "INSERT INTO blocks (block_id, block_height, block_hash, block_time, block_pub_time, block_prev_id, block_link, producer_id, producer_sig, txs, tx_root, us_state_root, cs_state_root, sg_root, aggz, certificate) VALUES (:block_id, :block_height, :block_hash, :block_time, :block_pub_time, :block_prev_id, :block_link, :producer_id, :producer_sig, :txs, :tx_root, :us_state_root, :cs_state_root, :sg_root, :aggz, :certificate)",
      soci::use(block_id), soci::use(block.getHeight()), soci::use(block_hash),
      soci::use(block.getBlockTime()), soci::use(block.getBlockPubTime()), soci::use(prev_block_id),
      soci::use(pre_block_sig), soci::use(block_prod_id),
      soci::use(block_pro_sig), soci::use(tx_id),
      soci::use(tx_root), soci::use(user_state_root),
      soci::use(contract_state_root), soci::use(sig_root), soci::use(aggz),
      soci::use(block_cert), soci::into(result));
  // clang-format on
  st.execute(true);

  return true;
}

// bool RdbController::updateData(const string &userId, const string &varType, const string &varName, const string &varValue) {
//  if (checkUserIdVarTypeVarName(&userId, &varType, &varName) == 0) {
//    string query = "UPDATE ledger SET var_value='" + varValue + "' WHERE user_id='" + userId + "' AND var_type='" + varType +
//                   "' AND var_name='" + varName + "'";
//    if (performQuery(query) == 0) {
//      logger::INFO("updateVarValue() function was processed!!!");
//      return 0;
//    } else {
//      logger::INFO("updateVarValue() function was not processed!!!");
//      return 1;
//    }
//  } else {
//    logger::INFO("updateVarValue() function was not processed!!!");
//    return 1;
//  }
//}
//
// bool RdbController::deleteData(const string &userId, const string &varType, const string &varName) {
//  if (checkUserIdVarTypeVarName(&userId, &varType, &varName) == 0) {
//    string query = "DELETE FROM ledger WHERE user_id='" + userId + "' AND var_type='" + varType + "' AND var_name='" + varName + "'";
//    if (performQuery(query) == 0) {
//      logger::INFO("deleteData() function was processed!!!");
//      return 0;
//    } else {
//      logger::INFO("deleteData() function was not processed!!!");
//      return 1;
//    }
//  } else {
//    logger::INFO("deleteData() function was not processed!!!");
//    return 1;
//  }
//}
}