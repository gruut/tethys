#ifndef TETHYS_PUBLIC_MERGER_TRANSACTION_HPP
#define TETHYS_PUBLIC_MERGER_TRANSACTION_HPP

#include "../../../../lib/json/include/json.hpp"
#include "../../../../lib/log/include/log.hpp"

#include "../../../../lib/tethys-utils/src/bytes_builder.hpp"
#include "../../../../lib/tethys-utils/src/sha256.hpp"
#include "../../../../lib/tethys-utils/src/type_converter.hpp"
#include "../config/storage_type.hpp"
#include "endorser.hpp"

#include <array>
#include <iostream>
#include <string>
#include <vector>

namespace tethys {
class Transaction {
private:
  base58_type m_txid;
  alphanumeric_type m_world;
  alphanumeric_type m_chain;
  timestamp_t m_tx_time;

  contract_id_type m_contract_id;
  base58_type m_receiver_id;
  int m_fee;             // msg_tx에서 받은, result 전의 값
  int m_fee_author;      // result 결과 처리 후
  int m_fee_user;        // result 결과 처리 후
  bytes m_tx_input_cbor; // to_cbor 된 상태

  base58_type m_tx_user_id;
  string m_tx_user_pk;
  base64_type m_tx_user_sig;
  base64_type m_tx_user_agga;
  base64_type m_tx_user_aggz;

  vector<Endorser> m_tx_endorsers;

  base64_type m_tx_agg_cbor;
  base58_type m_block_id;
  int m_tx_pos; // static_merkle_tree에서의 idx
  base64_type m_tx_output;

public:
  bool inputMsgTx(const nlohmann::json &msg_tx_json) {
    try {
      m_txid = msg_tx_json["/txid"_json_pointer];
      m_world = msg_tx_json["/world"_json_pointer];
      m_chain = msg_tx_json["/chain"_json_pointer];
      m_tx_time = static_cast<tethys::timestamp_t>(stoll(json::get<string>(msg_tx_json, "time").value()));

      m_contract_id = msg_tx_json["/body/cid"_json_pointer];
      m_receiver_id = msg_tx_json["/body/receiver"_json_pointer];
      m_fee = stoi(json::get<string>(msg_tx_json["body"], "fee").value());
      setTxInputCbor(msg_tx_json["body"]["input"]);

      m_tx_user_id = msg_tx_json["/user/id"_json_pointer];
      m_tx_user_pk = extractPubKeyIfUnauthorized(msg_tx_json);
      m_tx_user_sig = msg_tx_json["/user/sig"_json_pointer];

      if (!setEndorsers(msg_tx_json["endorser"])) {
        return false;
      }
      return true;
    } catch (nlohmann::json::parse_error &e) {
      logger::ERROR("Failed to parse transaction json: {}", e.what());
      return false;
    } catch (std::exception &e) {
      logger::ERROR("Unexpected error at `Transaction#inputMsgTx`: {}", e.what());
      return false;
    }
  }

  string extractPubKeyIfUnauthorized(const nlohmann::json &msg) {
    auto pk = msg["user"]["pk"];

    if (pk.is_null()) {
      return "";
    } else {
      return pk;
    }
  }

  bool inputMsgTxAgg(const nlohmann::json &tx_json, const block_info_type &block_info) {
    inputMsgTxAgg(tx_json);

    base64_type tmp_tx_agg_cbor = block_info.tx_agg_cbor;
    base58_type tmp_block_id = block_info.block_id;

    m_tx_agg_cbor = tmp_tx_agg_cbor;
    m_block_id = tmp_block_id;

    // TODO: 아래는 tx scope에는 저장되는 사항이지만, json으로 입력되는 내용은 아님. 보류.
    //    setTxPosition();
    //    setTxOutput(); - 이건 state tree를 설정할 때 받아서 넣을 수 있음. cbor형태의 query.result

    return true;
  }

  bool inputMsgTxAgg(const nlohmann::json &tx_json) {
    try {
      m_txid = json::get<string>(tx_json, "txid").value();
      m_tx_time = static_cast<tethys::timestamp_t>(stoll(json::get<string>(tx_json, "time").value()));

      m_contract_id = json::get<string>(tx_json["body"], "cid").value();
      m_receiver_id = json::get<string>(tx_json["body"], "receiver").value();
      m_fee = stoi(json::get<string>(tx_json["body"], "fee").value());
      setTxInputCbor(tx_json["body"]["input"]);

      m_tx_user_id = json::get<string>(tx_json["user"], "id").value();
      m_tx_user_pk = json::get<string>(tx_json["user"], "pk").value();
      m_tx_user_agga = json::get<string>(tx_json["user"], "a").value();
      m_tx_user_aggz = json::get<string>(tx_json["user"], "z").value();

      if (!setEndorsers(tx_json["endorser"])) {
        return false;
      }
      return true;
    } catch (nlohmann::json::parse_error &e) {
      logger::ERROR("Failed to parse transaction json: {}", e.what());
      return false;
    } catch (...) {
      logger::ERROR("Unexpected error at `Transaction#inputMsgTxAgg`");
      return false;
    }
  }

  void setTxInputCbor(const nlohmann::json &input_array) {
    m_tx_input_cbor = nlohmann::json::to_cbor(input_array);
  }

  bool setEndorsers(const nlohmann::json &endorser_array) {
    if (!endorser_array.is_array())
      return false;

    m_tx_endorsers.clear();
    for (auto &each_endorser : endorser_array) {
      if (json::get<string>(each_endorser, "sig").has_value())
        m_tx_endorsers.emplace_back(json::get<string>(each_endorser, "id").value(), json::get<string>(each_endorser, "pk").value(),
                                    json::get<string>(each_endorser, "sig").value());
      else
        m_tx_endorsers.emplace_back(json::get<string>(each_endorser, "id").value(), json::get<string>(each_endorser, "pk").value(),
                                    json::get<string>(each_endorser, "a").value(), json::get<string>(each_endorser, "z").value());
    }
    return true;
  }

  void setWorld(const string &world) {
    m_world = world;
  }

  void setChain(const string &chain) {
    m_chain = chain;
  }

  base58_type getTxId() const {
    return m_txid;
  }

  const string &getTxUserPk() const {
    return m_tx_user_pk;
  }

  const alphanumeric_type &getWorld() const {
    return m_world;
  }

  const alphanumeric_type &getChain() const {
    return m_chain;
  }

  timestamp_t getTxTime() const {
    return m_tx_time;
  }

  string getContractId() const {
    return m_contract_id;
  }

  base58_type getReceiverId() const {
    return m_receiver_id;
  }

  int getFee() const {
    return m_fee;
  }

  int getFeeAuthor() const {
    int m_fee_author;
  }

  int getFeeUser() const {
    int m_fee_user;
  }

  bytes getTxInputCbor() const {
    return m_tx_input_cbor;
  }

  base58_type getUserId() const {
    return m_tx_user_id;
  }

  base64_type getUserSig() const {
    return m_tx_user_sig;
  }

  base64_type getUserAgga() const {
    return m_tx_user_agga;
  }

  base64_type getUserAggz() const {
    return m_tx_user_aggz;
  }

  const vector<Endorser> &getEndorsers() const {
    return m_tx_endorsers;
  }

  base64_type getTxAggCbor() const {
    return m_tx_agg_cbor;
  }

  int getTxPos() const {
    return m_tx_pos;
  }
};
} // namespace tethys
#endif
