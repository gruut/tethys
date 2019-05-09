#include "include/db_controller.hpp"

using namespace std;

namespace gruut {

DBController::DBController(string_view dbms, string_view table_name, string_view db_user_id, string_view db_password)
    : m_dbms(dbms), m_table_name(table_name), m_db_user_id(db_user_id), m_db_password(db_password),
      m_db_pool(config::DB_SESSION_POOL_SIZE) {
  for (int i = 0; i != config::DB_SESSION_POOL_SIZE; ++i) {
    soci::session &sql = m_db_pool.at(i);
    sql.open(m_dbms, "service=" + m_table_name + " user=" + m_db_user_id + " password=" + m_db_password);
  }
  logger::INFO("DB pool initialize");
}

bool DBController::insertBlockData(Block &block) {
  logger::INFO("insert Block Data");

  soci::row result;
  soci::session db_session(m_db_pool);

  // clang-format off
  soci::statement st = (db_session.prepare << "INSERT INTO blocks (block_id, block_height, block_hash, block_time, block_pub_time, block_prev_id, block_link, producer_id, producer_sig, txs, tx_root, us_state_root, cs_state_root, sg_root, aggz, certificate) VALUES (:block_id, :block_height, :block_hash, :block_time, :block_pub_time, :block_prev_id, :block_link, :producer_id, :producer_sig, :txs, :tx_root, :us_state_root, :cs_state_root, :sg_root, :aggz, :certificate)",
      soci::use(block.getBlockId(), "block_id"), soci::use(block.getHeight(), "block_height"), soci::use(block.getBlockHash(), "block_hash"),
      soci::use(block.getBlockTime(), "block_time"), soci::use(block.getBlockPubTime(), "block_pub_time"), soci::use(block.getPrevBlockId(), "block_prev_id"),
      soci::use(block.getPrevBlockSig(), "block_link"), soci::use(block.getBlockProdId(), "producer_id"),
      soci::use(block.getBlockProdSig(),"producer_sig"), soci::use(block.getTransactions()[0].getTxid(),"txs"),
      soci::use(block.getTxRoot(),"tx_root"), soci::use(block.getUserStateRoot(),"us_state_root"),
      soci::use(block.getContractStateRoot(),"cs_state_root"), soci::use(block.getSgRoot(),"sg_root"), soci::use(block.getAggz(),"aggz"),
      soci::use(block.getBlockCert(), "certificate"), soci::into(result));
  // clang-format on
  st.execute(true);
  return true;
}

// bool DBController::updateData(const string &userId, const string &varType, const string &varName, const string &varValue) {
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
// bool DBController::deleteData(const string &userId, const string &varType, const string &varName) {
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