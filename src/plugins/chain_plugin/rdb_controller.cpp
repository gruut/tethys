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

bool RdbController::insertBlockData(Block &block) {
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
    logger::ERROR("Unexpected error at `insertBlockData`");
    return false;
  }
  return true;
}

bool RdbController::insertTransactionData(Block &block) {
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
      logger::ERROR("Unexpected error at `insertBlockData`");
      return false;
    }
  return true;
}

bool RdbController::insertUserLedgerData(std::map<string, user_ledger_type> &user_ledger) {
  for (auto &each_ledger : user_ledger) {
    // TODO: ledger에 있는걸 rdb 에 반영
    each_ledger.which_scope; // user scope
    each_ledger.var_name;
    each_ledger.var_val;
    each_ledger.var_type;
    each_ledger.var_owner; // uid
    each_ledger.up_time;
    each_ledger.up_block;
    each_ledger.tag; // user scope only
    each_ledger.pid;

    if (each_ledger.second.query_type == QueryType::INSERT) {
      queryInsert();
    } else if (each_ledger.second.query_type == QueryType::UPDATE) {
      queryUpdate();
    } else if (each_ledger.second.query_type == QueryType::DELETE) {
      queryDelete();
    }
  }
}

bool RdbController::insertContractLedgerData(std::map<string, contract_ledger_type> &contract_ledger) {
  for (auto &each_ledger : contract_ledger) {
    // TODO: ledger에 있는걸 rdb 에 반영
    each_ledger.which_scope; // contract scope
    each_ledger.var_name;
    each_ledger.var_val;
    each_ledger.var_type;
    each_ledger.var_owner; // cid
    each_ledger.up_time;
    each_ledger.up_block;
    each_ledger.var_info; // contract scope only
    each_ledger.pid;

    if (each_ledger.second.query_type == QueryType::INSERT) {
      queryInsert();
    } else if (each_ledger.second.query_type == QueryType::UPDATE) {
      queryUpdate();
    } else if (each_ledger.second.query_type == QueryType::DELETE) {
      queryDelete();
    }
  }
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

bool RdbController::queryUserJoin(UnresolvedBlock &UR_block, nlohmann::json &option, result_query_info_type &result_info) {
  /// unresolved block pool과 관계 없이 즉시 적용
  try {
    base58_type uid = result_info.user;
    timestamp_t register_day = static_cast<gruut::timestamp_t>(stoll(json::get<string>(option, "register_day").value()));
    string register_code = json::get<string>(option, "register_code").value();
    int gender = stoi(json::get<string>(option, "gender").value());
    string isc_type = json::get<string>(option, "isc_type").value();
    string isc_code = json::get<string>(option, "isc_code").value();
    string location = json::get<string>(option, "location").value();
    int age_limit = stoi(json::get<string>(option, "age_limit").value());

    string msg = uid + to_string(register_day) + register_code + to_string(gender) + isc_type + isc_code + location + to_string(age_limit);

    // TODO: signature by Gruut Authority 정확히 구현
    AGS ags;
    /// auto sigma = ags.sign(GruutAuthority_secret_key, msg);
    string sigma = "TODO: signature by Gruut Authority";

    // clang-format off
    soci::row result;
    soci::session db_session(RdbController::pool());

    soci::statement st = (db_session.prepare << "INSERT INTO user_attributes (uid, register_day, register_code, gender, isc_type, isc_code, location, age_limit, sigma) VALUES (:uid, :register_day, :register_code, :gender, :isc_type, :isc_code, :location, :age_limit, :sigma)",
        soci::use(uid), soci::use(register_day), soci::use(register_code),
        soci::use(gender), soci::use(isc_type), soci::use(isc_code),
        soci::use(location), soci::use(age_limit), soci::use(sigma),
        soci::into(result));
    // clang-format on
    st.execute(true);
  } catch (soci::mysql_soci_error const &e) {
    logger::ERROR("MySQL error: {}", e.what());
    return false;
  } catch (...) {
    logger::ERROR("Unexpected error at `queryUserJoin`");
    return false;
  }
  return true;
}

bool RdbController::queryUserCert(UnresolvedBlock &UR_block, nlohmann::json &option, result_query_info_type &result_info) {
  /// unresolved block pool과 관계 없이 즉시 적용
  try {
    base58_type uid = result_info.user;
    string sn = json::get<string>(option, "sn").value();
    timestamp_t nvbefore = static_cast<uint64_t>(stoll(json::get<string>(option, "notbefore").value()));
    timestamp_t nvafter = static_cast<uint64_t>(stoll(json::get<string>(option, "notafter").value()));
    string x509 = json::get<string>(option, "x509").value();

    // clang-format off
    soci::row result;
    soci::session db_session(RdbController::pool());

    soci::statement st = (db_session.prepare << "INSERT INTO user_certificates (uid, sn, nvbefore, nvafter, x509) VALUES (:uid, :sn, :nvbefore, :nvafter, :x509)",
        soci::use(uid), soci::use(sn), soci::use(nvbefore), soci::use(nvafter), soci::use(x509),
        soci::into(result));
    // clang-format on
    st.execute(true);
  } catch (soci::mysql_soci_error const &e) {
    logger::ERROR("MySQL error: {}", e.what());
    return false;
  } catch (...) {
    logger::ERROR("Unexpected error at `queryUserCert`");
    return false;
  }
  return true;
}

bool RdbController::queryContractNew(UnresolvedBlock &UR_block, nlohmann::json &option, result_query_info_type &result_info) {
  // TODO: ContractNew는 resolve 대기 필요
  try {
    contract_id_type cid = json::get<string>(option, "cid").value();
    timestamp_t after = static_cast<uint64_t>(stoll(json::get<string>(option, "after").value()));
    timestamp_t before = static_cast<uint64_t>(stoll(json::get<string>(option, "before").value()));
    base58_type author = json::get<string>(option, "author").value();

    // TODO: friend 추가할 때 자신을 friend로 추가한 contract를 찾아서 추가해야함을 주의
    contract_id_type friends = json::get<string>(option, "friend").value();

    string contract = json::get<string>(option, "contract").value();
    string desc = json::get<string>(option, "desc").value();
    string sigma = json::get<string>(option, "sigma").value();

    // clang-format off
    soci::row result;
    soci::session db_session(RdbController::pool());

    soci::statement st = (db_session.prepare << "INSERT INTO contracts (cid, after, before, author, friends, contract, desc, sigma) VALUES (:cid, :after, :before, :author, :friends, :contract, :desc, :sigma)",
        soci::use(cid), soci::use(after), soci::use(before), soci::use(author), soci::use(friends),
        soci::use(contract), soci::use(desc), soci::use(sigma),
        soci::into(result));
    // clang-format on
    st.execute(true);
  } catch (soci::mysql_soci_error const &e) {
    logger::ERROR("MySQL error: {}", e.what());
    return false;
  } catch (...) {
    logger::ERROR("Unexpected error at `queryContractNew`");
    return false;
  }
  return true;
}

bool RdbController::queryContractDisable(UnresolvedBlock &UR_block, nlohmann::json &option, result_query_info_type &result_info) {
  // TODO: ContractDisable은 resolve 대기 필요
  try {
    // TODO: $user = cid.author인 경우에만 허용
    contract_id_type cid = json::get<string>(option, "cid").value();
    timestamp_t before = TimeUtil::nowBigInt();

    // clang-format off
    soci::row result;
    soci::session db_session(RdbController::pool());

    soci::statement st = (db_session.prepare << "UPDATE contracts SET before = :before WHERE cid = :cid",
        soci::use(cid, "cid"), soci::use(before, "before"),
        soci::into(result));
    // clang-format on
    st.execute(true);
  } catch (soci::mysql_soci_error const &e) {
    logger::ERROR("MySQL error: {}", e.what());
    return false;
  } catch (...) {
    logger::ERROR("Unexpected error at `queryContractDisable`");
    return false;
  }
  return true;
}

bool RdbController::queryIncinerate(std::map<string, user_ledger_type> &user_ledger, nlohmann::json &option, result_query_info_type &result_info) {
  // TODO: m_mem_ledger 사용하여 갱신값 계산. resolve 대기 필요
  try {
    string amount = json::get<string>(option, "amount").value();
    string pid = json::get<string>(option, "pid").value();

    // TODO: pid가 가리키는 user가 자신인지를 검사해야 함. 다른 유저의 변수를 삭제하면 안 됨.

    // clang-format off
    soci::row result;
    soci::session db_session(RdbController::pool());

    soci::statement st = (db_session.prepare << "UPDATE user_scope SET var_value = :var_value WHERE pid = :pid",
        soci::use(amount, "var_value"), soci::use(pid, "pid"),
        soci::into(result));
    // clang-format on
    st.execute(true);
  } catch (soci::mysql_soci_error const &e) {
    logger::ERROR("MySQL error: {}", e.what());
    return false;
  } catch (...) {
    logger::ERROR("Unexpected error at `queryIncinerate`");
    return false;
  }
  return true;
}

bool RdbController::queryCreate(std::map<string, user_ledger_type> &user_ledger, nlohmann::json &option, result_query_info_type &result_info) {
  // TODO: m_mem_ledger 사용하여 갱신값 계산
  try {
    string amount = json::get<string>(option, "amount").value();
    string var_name = json::get<string>(option, "name").value();
    string var_type = json::get<string>(option, "type").value();
    string tag = json::get<string>(option, "tag").value();
    string var_owner = result_info.user;
    timestamp_t up_time = TimeUtil::nowBigInt(); // TODO: DB에 저장되는 시간인지, mem_ledger에 들어오는 시간인지
    block_height_type up_block = result_info.block_height;
    string pid = ""; // TODO: mem_ledger에서 계산됨. 연동 필요.

    // clang-format off
    soci::row result;
    soci::session db_session(RdbController::pool());

    soci::statement st = (db_session.prepare << "INSERT INTO user_scope (var_name, var_value, var_type, var_owner, up_time, up_block, tag, pid) VALUES (:var_name, :var_value, :var_type, :var_owner, :up_time, :up_block, :tag, :pid)",
        soci::use(var_name), soci::use(amount), soci::use(var_type),
        soci::use(var_owner), soci::use(up_time), soci::use(up_block),
        soci::use(tag), soci::use(pid),
        soci::into(result));
    // clang-format on
    st.execute(true);
  } catch (soci::mysql_soci_error const &e) {
    logger::ERROR("MySQL error: {}", e.what());
    return false;
  } catch (...) {
    logger::ERROR("Unexpected error at `queryCreate`");
    return false;
  }
  return true;
}

bool RdbController::queryTransfer(std::vector<LedgerRecord> &mem_ledger, nlohmann::json &option, result_query_info_type &result_info) {
  // TODO: m_mem_ledger 사용하여 갱신값 계산
  //    from과 to가 user/contract 따라 처리될 수 있도록 구현 필요

  try {
    base58_type from = json::get<string>(option, "from").value();
    base58_type to = json::get<string>(option, "to").value();
    string amount = json::get<string>(option, "amount").value();
    string unit = json::get<string>(option, "unit").value();
    string pid = json::get<string>(option, "pid").value();
    string tag = json::get<string>(option, "tag").value();

    // pid는 from의 검색 조건이며, tag는 to에게 보낼 때의 tag 지정을 의미한다.
    // to가 contract일 경우, tag는 무시되고 contract의 var_info에 from이 기록된다.
    // TODO: transfer되어서 새로운 변수가 생겼으면 insert, 갱신이면 update 요청이 된다. mem_ledger쪽과 연계해야 함.

    // clang-format off
    soci::row result;
    soci::session db_session(RdbController::pool());

    // user_scope insert, update, contract_scope insert, update

    soci::statement st = (db_session.prepare << "UPDATE user_scope SET var_value = :var_value WHERE pid = :pid",
        soci::use(amount, "var_value"), soci::use(pid, "pid"),
        soci::into(result));
    // clang-format on
    st.execute(true);
  } catch (soci::mysql_soci_error const &e) {
    logger::ERROR("MySQL error: {}", e.what());
    return false;
  } catch (...) {
    logger::ERROR("Unexpected error at `queryTransfer`");
    return false;
  }
  return true;
}

bool RdbController::queryUserScope(std::map<string, user_ledger_type> &user_ledger, nlohmann::json &option, result_query_info_type &result_info) {
  // TODO: m_mem_ledger 사용하여 갱신값 계산
  try {
    string var_name = json::get<string>(option, "name").value();
    string target = json::get<string>(option, "for").value(); // user or author
    string var_value = json::get<string>(option, "value").value();
    string pid = json::get<string>(option, "pid").value();
    string tag = json::get<string>(option, "tag").value();
    base58_type uid;

    if (target == "user") {
      uid = result_info.user;
    } else if (target == "author") {
      uid = result_info.author;
    } else {
      logger::ERROR("target is not 'user' or 'author' at `queryUserScope`");
    }

    // TODO: '통화'속성의 변수는 변경 불가능한 조건 검사 시행
    // TODO: pid의 계산방법, 반영법, for의 정확한 활용 등을 적용

    // clang-format off
    soci::row result;
    soci::session db_session(RdbController::pool());

    soci::statement st = (db_session.prepare << "UPDATE user_scope SET var_name = :var_name, var_value = :var_value, tag = :tag, WHERE pid = :pid",
        soci::use(var_name, "var_name"), soci::use(var_value, "var_value"), soci::use(tag, "tag"),
        soci::into(result));
    // clang-format on
    st.execute(true);
  } catch (soci::mysql_soci_error const &e) {
    logger::ERROR("MySQL error: {}", e.what());
    return false;
  } catch (...) {
    logger::ERROR("Unexpected error at `queryUserScope`");
    return false;
  }

  return true;
}

bool RdbController::queryContractScope(std::map<string, contract_ledger_type> &contract_ledger, nlohmann::json &option, result_query_info_type &result_info) {
  // TODO: m_mem_ledger 사용하여 갱신값 계산
  try {
    string var_name = mem_ledger string var_value = json::get<string>(option, "value").value();
    contract_id_type cid = json::get<string>(option, "cid").value();
    string pid = json::get<string>(option, "pid").value();

    // TODO: '통화'속성의 변수는 변경 불가능한 조건 검사 시행
    // TODO: pid의 계산방법, 반영법, for의 정확한 활용 등을 적용

    // clang-format off
    soci::row result;
    soci::session db_session(RdbController::pool());

    soci::statement st = (db_session.prepare << "UPDATE contracts SET var_value = :var_value WHERE pid = :pid",
        soci::use(var_name, "var_name"), soci::use(var_value, "var_value"),
        soci::into(result));
    // clang-format on
    st.execute(true);
  } catch (soci::mysql_soci_error const &e) {
    logger::ERROR("MySQL error: {}", e.what());
    return false;
  } catch (...) {
    logger::ERROR("Unexpected error at `queryContractScope`");
    return false;
  }
  return true;
}

bool RdbController::queryTradeItem(std::vector<LedgerRecord> &mem_ledger, nlohmann::json &option, result_query_info_type &result_info) {
  nlohmann::json costs = option["costs"];
  nlohmann::json units = option["units"];
  base58_type author = json::get<string>(option, "author").value();
  base58_type user = json::get<string>(option, "user").value();
  string pid = json::get<string>(option, "pid").value();

  // TODO: 처리 절차 구현할 것
  return true;
}

bool RdbController::queryTradeVal(std::vector<LedgerRecord> &mem_ledger, nlohmann::json &option, result_query_info_type &result_info) {
  // TODO: TBA (19.05.21 현재 문서에 관련 사항 미기재)
  return true;
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

} // namespace gruut