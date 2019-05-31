#include "include/rdb_controller.hpp"
#include <ags.hpp>

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

bool RdbController::applyBlock(Block &block) {
  logger::INFO("insert Block Data");

  soci::row result;
  soci::session db_session(RdbController::pool());

  try {
    // clang-format off
  string block_id = block.getBlockId();
  string block_hash = block.getBlockHash();
  string prev_block_id = block.getPrevBlockId();
  string pre_block_sig = block.getPrevBlockSig();
  string block_prod_id = block.getBlockProdId();
  string block_pro_sig = block.getBlockProdSig();
  string tx_id = block.getTransactions()[0].getTxId();
  string tx_root = block.getTxRoot();
  string user_state_root = block.getUserStateRoot();
  string contract_state_root = block.getContractStateRoot();
  string sig_root = block.getSgRoot();
  string block_cert = block.getBlockCert();

  soci::statement st = (db_session.prepare << "INSERT INTO blocks (block_id, block_height, block_hash, block_time, block_pub_time, block_prev_id, block_link, producer_id, producer_sig, txs, tx_root, us_state_root, cs_state_root, sg_root, certificate) VALUES (:block_id, :block_height, :block_hash, :block_time, :block_pub_time, :block_prev_id, :block_link, :producer_id, :producer_sig, :txs, :tx_root, :us_state_root, :cs_state_root, :sg_root, :certificate)",
      soci::use(block_id), soci::use(block.getHeight()), soci::use(block_hash),
      soci::use(block.getBlockTime()), soci::use(block.getBlockPubTime()), soci::use(prev_block_id),
      soci::use(pre_block_sig), soci::use(block_prod_id),
      soci::use(block_pro_sig), soci::use(tx_id),
      soci::use(tx_root), soci::use(user_state_root),
      soci::use(contract_state_root), soci::use(sig_root),
      soci::use(block_cert), soci::into(result));
    // clang-format on
    st.execute(true);
  } catch (soci::mysql_soci_error const &e) {
    logger::ERROR("MySQL error: {}", e.what());
    return false;
  } catch (...) {
    logger::ERROR("Unexpected error at `applyBlock`");
    return false;
  }
  return true;
}

bool RdbController::applyTransaction(Block &block) {
  logger::INFO("insert Transaction Data");

  soci::row result;
  soci::session db_session(RdbController::pool());
  for (auto &each_transaction : block.getTransactions())
    try {
      // clang-format off
        string tx_id = each_transaction.getTxId();
        string tx_contract_id = each_transaction.getContractId();
        string tx_user = each_transaction.getUserId();
        string tx_user_pk = each_transaction.getTxUserPk();
        string tx_receiver = each_transaction.getReceiverId();
        string tx_input = TypeConverter::bytesToString(each_transaction.getTxInputCbor());
        string tx_agg_cbor = each_transaction.getTxAggCbor();
        string tx_block_id = block.getBlockId();

        soci::statement st = (db_session.prepare << "INSERT INTO transactions (tx_id, tx_time, tx_contract_id, tx_fee_author, tx_fee_user, tx_user, tx_user_pk, tx_receiver, tx_input, tx_agg_cbor, block_id, tx_pos) VALUES (:tx_id, :tx_time, :tx_contract_id, :tx_fee_author, :tx_fee_user, :tx_user, :tx_user_pk, :tx_receiver, :tx_input, :tx_agg_cbor, :block_id, :tx_pos)",
            soci::use(tx_id), soci::use(each_transaction.getTxTime()), soci::use(tx_contract_id),
            soci::use(each_transaction.getFee()), soci::use(each_transaction.getFee()), soci::use(tx_user),
            soci::use(tx_user_pk), soci::use(tx_receiver),
            soci::use(tx_input), soci::use(tx_agg_cbor),
            soci::use(tx_block_id), soci::use(each_transaction.getTxPos()),
            soci::into(result));
      // clang-format on
      st.execute(true);
    } catch (soci::mysql_soci_error const &e) {
      logger::ERROR("MySQL error: {}", e.what());
      return false;
    } catch (...) {
      logger::ERROR("Unexpected error at `applyTransaction`");
      return false;
    }
  return true;
}

bool RdbController::applyUserLedger(std::map<string, user_ledger_type> &user_ledger_list) {
  try {
    string var_name;
    string var_val;
    int var_type;
    base58_type uid;
    timestamp_t up_time;
    block_height_type up_block;
    string tag;
    hash_t pid;

    // TODO: '통화'속성의 변수는 변경 불가능한 조건 검사 시행
    // TODO: pid의 계산방법, 반영법, for의 정확한 활용 등을 적용

    for (auto &each_ledger : user_ledger_list) {
      soci::row result;
      soci::session db_session(RdbController::pool());
      soci::statement st(db_session);

      // clang-format off
      if (each_ledger.second.query_type == QueryType::INSERT) {
        st = (db_session.prepare << "INSERT INTO user_scope (var_name, var_value, var_type, var_owner, up_time, up_block, tag, pid) VALUES (:var_name, :var_value, :var_type, :var_owner, :up_time, :up_block, :tag, :pid)",
                soci::use(var_name, "var_name"), soci::use(var_val, "var_value"), soci::use(tag, "tag"),
                soci::into(result));
      } else if (each_ledger.second.query_type == QueryType::UPDATE) {
        st = (db_session.prepare << "UPDATE user_scope SET var_name = :var_name, var_value = :var_value, tag = :tag, WHERE pid = :pid",
                soci::use(var_name, "var_name"), soci::use(var_val, "var_value"), soci::use(tag, "tag"),
                soci::into(result));
      } else if ((each_ledger.second.query_type == QueryType::DELETE) && (each_ledger.second.var_val == "")) {
        st = (db_session.prepare << "DELETE FROM user_scope WHERE ",
                soci::use(var_name, "var_name"), soci::use(var_val, "var_value"), soci::use(tag, "tag"),
                soci::into(result));
      } else
        logger::ERROR("Error at applyUserLedger");
      // clang-format on

      st.execute(true);
    }
  } catch (soci::mysql_soci_error const &e) {
    logger::ERROR("MySQL error: {}", e.what());
    return false;
  } catch (...) {
    logger::ERROR("Unexpected error at applyUserLedger");
    return false;
  }
  return true;
}

bool RdbController::applyContractLedger(std::map<string, contract_ledger_type> &contract_ledger_list) {
  try {
    string var_name;
    string var_val;
    int var_type;
    base58_type uid;
    timestamp_t up_time;
    block_height_type up_block;
    string var_info;
    hash_t pid;

    // TODO: '통화'속성의 변수는 변경 불가능한 조건 검사 시행
    // TODO: pid의 계산방법, 반영법, for의 정확한 활용 등을 적용

    for (auto &each_ledger : contract_ledger_list) {
      soci::row result;
      soci::session db_session(RdbController::pool());
      soci::statement st(db_session);

      // clang-format off
      if (each_ledger.second.query_type == QueryType::INSERT) {
        st = (db_session.prepare << "INSERT INTO contract_scope (var_name, var_value, var_type, var_owner, up_time, up_block, tag, pid) VALUES (:var_name, :var_value, :var_type, :var_owner, :up_time, :up_block, :tag, :pid)",
            soci::use(var_name, "var_name"), soci::use(var_val, "var_value"), soci::use(tag, "tag"),
            soci::into(result));
      } else if (each_ledger.second.query_type == QueryType::UPDATE) {
        st = (db_session.prepare << "UPDATE contract_scope SET var_name = :var_name, var_value = :var_value, tag = :tag, WHERE pid = :pid",
            soci::use(var_name, "var_name"), soci::use(var_val, "var_value"), soci::use(tag, "tag"),
            soci::into(result));
      } else if ((each_ledger.second.query_type == QueryType::DELETE) && (each_ledger.second.var_val == "")) {
        st = (db_session.prepare << "DELETE FROM contract_scope WHERE ",
            soci::use(var_name, "var_name"), soci::use(var_val, "var_value"), soci::use(tag, "tag"),
            soci::into(result));
      } else
        logger::ERROR("Error at applyContractLedger");
      // clang-format on

      st.execute(true);
    }
  } catch (soci::mysql_soci_error const &e) {
    logger::ERROR("MySQL error: {}", e.what());
    return false;
  } catch (...) {
    logger::ERROR("Unexpected error at applyContractLedger");
    return false;
  }
  return true;
}

bool RdbController::applyUserAttribute(std::map<base58_type, user_attribute_type> &user_attribute_list) {
  try {
    base58_type uid;
    timestamp_t register_day;
    string register_code;
    int gender;
    string isc_type;
    string isc_code;
    string location;
    int age_limit;

    // TODO: INSERT일수도, UPDATE일수도 있음. 구분 필요.

    for (auto &each_attribute : user_attribute_list) {
      soci::row result;
      soci::session db_session(RdbController::pool());
      soci::statement st(db_session);

      // clang-format off
      if (each_attribute.second.query_type == QueryType::INSERT) {
        st = (db_session.prepare << "INSERT INTO user_attributes (var_name, var_value, var_type, var_owner, up_time, up_block, tag, pid) VALUES (:var_name, :var_value, :var_type, :var_owner, :up_time, :up_block, :tag, :pid)",
            soci::use(var_name, "var_name"), soci::use(var_val, "var_value"), soci::use(tag, "tag"),
            soci::into(result));
      } else if (each_attribute.second.query_type == QueryType::UPDATE) {
        st = (db_session.prepare << "UPDATE user_attributes SET var_name = :var_name, var_value = :var_value, tag = :tag, WHERE pid = :pid",
            soci::use(var_name, "var_name"), soci::use(var_val, "var_value"), soci::use(tag, "tag"),
            soci::into(result));
      } else if ((each_attribute.second.query_type == QueryType::DELETE) && (each_attribute.second.var_val == "")) {
        st = (db_session.prepare << "DELETE FROM user_attributes WHERE ",
            soci::use(var_name, "var_name"), soci::use(var_val, "var_value"), soci::use(tag, "tag"),
            soci::into(result));
      } else
        logger::ERROR("Error at applyUserAttribute");
      // clang-format on

      st.execute(true);
    }
  } catch (soci::mysql_soci_error const &e) {
    logger::ERROR("MySQL error: {}", e.what());
    return false;
  } catch (...) {
    logger::ERROR("Unexpected error at applyUserAttribute");
    return false;
  }
  return true;
}

bool RdbController::applyUserCert(std::map<base58_type, user_cert_type> &user_cert_list) {
  try {
    base58_type uid;
    string sn;
    timestamp_t nvbefore;
    timestamp_t nvafter;
    string x509;

    // TODO: INSERT일수도, UPDATE일수도 있음. 구분 필요.
    for (auto &each_cert : user_cert_list) {
      soci::row result;
      soci::session db_session(RdbController::pool());
      soci::statement st(db_session);

      // clang-format off
      if (each_cert.second.query_type == QueryType::INSERT) {
        st = (db_session.prepare << "INSERT INTO user_certificates (var_name, var_value, var_type, var_owner, up_time, up_block, tag, pid) VALUES (:var_name, :var_value, :var_type, :var_owner, :up_time, :up_block, :tag, :pid)",
            soci::use(var_name, "var_name"), soci::use(var_val, "var_value"), soci::use(tag, "tag"),
            soci::into(result));
      } else if (each_cert.second.query_type == QueryType::UPDATE) {
        st = (db_session.prepare << "UPDATE user_certificates SET var_name = :var_name, var_value = :var_value, tag = :tag, WHERE pid = :pid",
            soci::use(var_name, "var_name"), soci::use(var_val, "var_value"), soci::use(tag, "tag"),
            soci::into(result));
      } else if ((each_cert.second.query_type == QueryType::DELETE) && (each_cert.second.var_val == "")) {
        st = (db_session.prepare << "DELETE FROM user_certificates WHERE ",
            soci::use(var_name, "var_name"), soci::use(var_val, "var_value"), soci::use(tag, "tag"),
            soci::into(result));
      } else
        logger::ERROR("Error at applyUserCert");
      // clang-format on

      st.execute(true);
    }
  } catch (soci::mysql_soci_error const &e) {
    logger::ERROR("MySQL error: {}", e.what());
    return false;
  } catch (...) {
    logger::ERROR("Unexpected error at applyUserCert");
    return false;
  }
  return true;
}

bool RdbController::applyContract(std::map<base58_type, contract_type> &contract_list) {
  try {
    contract_id_type cid;
    timestamp_t after;
    timestamp_t before;
    base58_type author;
    // TODO: friend 추가할 때 자신을 friend로 추가한 contract를 찾아서 추가해야함을 주의
    contract_id_type friends;
    string contract;
    string desc;
    string sigma;

    // TODO: INSERT일수도, UPDATE일수도 있음. 구분 필요.

    for (auto &each_contract : contract_list) {
      soci::row result;
      soci::session db_session(RdbController::pool());
      soci::statement st(db_session);

      // clang-format off
      if (each_contract.second.query_type == QueryType::INSERT) {
        st = (db_session.prepare << "INSERT INTO contracts (var_name, var_value, var_type, var_owner, up_time, up_block, tag, pid) VALUES (:var_name, :var_value, :var_type, :var_owner, :up_time, :up_block, :tag, :pid)",
            soci::use(var_name, "var_name"), soci::use(var_val, "var_value"), soci::use(tag, "tag"),
            soci::into(result));
      } else if (each_contract.second.query_type == QueryType::UPDATE) {
        st = (db_session.prepare << "UPDATE contracts SET var_name = :var_name, var_value = :var_value, tag = :tag, WHERE pid = :pid",
            soci::use(var_name, "var_name"), soci::use(var_val, "var_value"), soci::use(tag, "tag"),
            soci::into(result));
      } else if ((each_contract.second.query_type == QueryType::DELETE) && (each_contract.second.var_val == "")) {
        st = (db_session.prepare << "DELETE FROM contracts WHERE ",
            soci::use(var_name, "var_name"), soci::use(var_val, "var_value"), soci::use(tag, "tag"),
            soci::into(result));
      } else
        logger::ERROR("Error at applyContract");
      // clang-format on

      st.execute(true);
    }
  } catch (soci::mysql_soci_error const &e) {
    logger::ERROR("MySQL error: {}", e.what());
    return false;
  } catch (...) {
    logger::ERROR("Unexpected error at applyContract");
    return false;
  }
  return true;
}

vector<Block> RdbController::getBlocks(const string &condition) {
  try {
    soci::session db_session(RdbController::pool());
    soci::rowset<soci::row> rs = (db_session.prepare << "select * from blocks where " + condition);

    vector<Block> blocks;
    blocks.reserve(distance(rs.begin(), rs.end()));
    for (auto it = rs.begin(); it != rs.end(); ++it) {
      soci::row const &row = *it;
      Block block = rowToBlock(row);
      blocks.push_back(block);
    }

    return blocks;
  } catch (const std::exception &e) {
    logger::ERROR("Failed to get blocks: {}", e.what());

    throw e;
  }
}

Block RdbController::getBlock(const string &condition) {
  try {
    soci::session db_session(RdbController::pool());
    soci::row r;

    db_session << "select * from blocks " + condition, into(r);
    auto block = rowToBlock(r);

    return block;
  } catch (const std::exception &e) {
    logger::ERROR("Failed to get blocks: {}", e.what());

    throw e;
  }
}

Block RdbController::rowToBlock(const soci::row &r) {
  Block block;

  block.setBlockId(r.get<base58_type>("block_id"));
  block.setHeight(r.get<block_height_type>("block_height"));
  block.setBlockHash(r.get<base64_type>("block_hash"));
  block.setBlockTime(r.get<long long>("block_time"));
  block.setBlockPubTime(r.get<long long>("block_pub_time"));
  block.setBlockPrevId(r.get<base58_type>("block_prev_id"));
  block.setBlockPrevSig(r.get<base64_type>("block_link"));
  block.setProducerId(r.get<base58_type>("producer_id"));
  block.setProducerSig(r.get<base64_type>("producer_sig"));
  // TODO: TxID
  block.setTxRoot(r.get<string>("tx_root"));
  block.setUsStateRoot(r.get<string>("us_state_root"));
  block.setCsStateRoot(r.get<string>("cs_state_root"));
  block.setSgRoot(r.get<string>("sg_root"));

  nlohmann::json certificates = nlohmann::json(r.get<string>("certificate"));
  block.setUserCerts(certificates);

  return block;
}

string RdbController::getUserCert(const base58_type &user_id) {
  try {
    string user_cert = {};
    soci::session db_session(RdbController::pool());
    soci::statement st = (db_session.prepare << "select x509 from user_certificates where uid = " + user_id, soci::into(user_cert));
    st.execute(true);

    return user_cert;
  } catch (const std::exception &e) {
    logger::ERROR("Failed to get user_cert: {}", e.what());
    return string();
  }
}

bool RdbController::queryRunQuery(std::vector<LedgerRecord> &mem_ledger, nlohmann::json &option, result_query_info_type &result_info) {
  string type = json::get<string>(option, "type").value();
  nlohmann::json query = option["query"];
  timestamp_t after = static_cast<uint64_t>(stoll(json::get<string>(option, "after").value()));

  if ((type == "run.query") || (type == "user.cert")) {
    logger::ERROR("run.query cannot excute 'run.query' or 'user.cert'!");
    return false;
  }
  // TODO: Scheduler에게 지연 처리 요청 전송
  return true;
}

bool RdbController::queryRunContract(std::vector<LedgerRecord> &mem_ledger, nlohmann::json &option, result_query_info_type &result_info) {
  contract_id_type cid = json::get<string>(option, "cid").value();
  string input = json::get<string>(option, "input").value();
  timestamp_t after = static_cast<uint64_t>(stoll(json::get<string>(option, "after").value()));

  // TODO: authority.user를 현재 user로 대체하여 Scheduler에게 요청 전송
  return true;
}

bool RdbController::checkUnique() {
  soci::row result;
  soci::session db_session(RdbController::pool());
  soci::statement st = (db_session.prepare << "SELECT * from user_scope WHERE pid = :pid", soci::use(pid, "pid"), soci::into(result));
  st.execute(true);

  int num_rows = (int)st.get_affected_rows();
  if (num_rows == 1)
    return true;
  else
    return false;
}

bool RdbController::findUserFromRDB(string pid, user_ledger_type &user_ledger) {
  try {
    soci::row result;
    soci::session db_session(RdbController::pool());
    soci::statement st = (db_session.prepare << "SELECT * from user_scope WHERE pid = :pid", soci::use(pid, "pid"), soci::into(result));
    st.execute(true);

    // TODO: SELECT문으로 얻은 결과를 user_ledger로 반영하면 reference이기 때문에 반영되어 전달

  } catch (soci::mysql_soci_error const &e) {
    logger::ERROR("MySQL error: {}", e.what());
    return false;
  } catch (...) {
    logger::ERROR("Unexpected error at `queryUserScope`");
    return false;
  }
  return true;
}

} // namespace gruut