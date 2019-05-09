#ifndef GRUUT_PUBLIC_MERGER_TRANSACTION_HPP
#define GRUUT_PUBLIC_MERGER_TRANSACTION_HPP

#include "../../../../lib/json/include/json.hpp"
#include "../../../../lib/log/include/log.hpp"

#include "../../../../lib/gruut-utils/src/bytes_builder.hpp"
#include "../../../../lib/gruut-utils/src/sha256.hpp"
#include "../../../../lib/gruut-utils/src/type_converter.hpp"
#include "../config/storage_type.hpp"
#include "endorser.hpp"

#include <array>
#include <iostream>
#include <string>
#include <vector>

namespace gruut {
class Transaction {
private:
  base58_type m_txid;
  string m_world;
  string m_chain;
  timestamp_t m_tx_time;

  string m_contract_id;
  base58_type m_receiver_id;
  int m_fee;             // TODO: 처리 절차에 따라 author과 user로 나눠야 할 수 있음
  bytes m_tx_input_cbor; // to_cbor 된 상태

  base58_type m_tx_user_id;
  string m_tx_user_pk;
  base64_type m_tx_user_sig;

  vector<Endorser> m_tx_endorsers;

  base64_type m_tx_agg_cbor;
  base58_type m_block_id;
  int m_tx_pos; // static_merkle_tree에서의 idx
  base64_type m_tx_output;

public:
  bool setJson(const nlohmann::json &tx_json) {
    try {
      m_txid = json::get<string>(tx_json, "txid").value();

      m_world = tx_json["/world"_json_pointer];
      m_chain = tx_json["/chain"_json_pointer];

      m_tx_time = static_cast<gruut::timestamp_t>(stoll(json::get<string>(tx_json, "time").value()));

      m_contract_id = json::get<string>(tx_json["body"], "cid").value();
      m_receiver_id = json::get<string>(tx_json["body"], "receiver").value();
      m_fee = stoi(json::get<string>(tx_json["body"], "fee").value());
      setTxInputCbor(tx_json["body"]["input"]);

      m_tx_user_id = json::get<string>(tx_json["user"], "id").value();
      m_tx_user_pk = json::get<string>(tx_json["user"], "pk").value();
      m_tx_user_sig = json::get<string>(tx_json["user"], "sig").value();

      if(!setEndorsers(tx_json["endorser"])) {
        return false;
      }

      // TODO: 아래 넷은 tx scope에는 저장되는 사항이지만, json으로 입력되는 내용은 아님. 보류.
      //    setTxAggCbor();
      //    setBlockId();
      //    setTxPosition();
      //    setTxOutput();
      return true;
    } catch(nlohmann::json::parse_error &e) {
      logger::ERROR("Failed to parse transaction json: {}", e.what());
      return false;
    } catch(...) {
      logger::ERROR("Unexpected error at `Transaction#setJson`");
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
      Endorser endors;

      endors.endorser_id = json::get<string>(each_endorser, "id").value();
      endors.endorser_pk = json::get<string>(each_endorser, "pk").value();
      endors.endorser_signature = json::get<string>(each_endorser, "sig").value();
      m_tx_endorsers.push_back(endors);
    }
    return true;
  }

  base58_type getTxID() const {
    return m_txid;
  }

  const string &getTxUserPk() const {
    return m_tx_user_pk;
  }

  const string &getWorld() const {
    return m_world;
  }
  const string &getChain() const {
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

  bytes getTxInputCbor() const {
    return m_tx_input_cbor;
  }

  base58_type getUserId() const {
    return m_tx_user_id;
  }

  base64_type getUserSig() const {
    return m_tx_user_sig;
  }

  const vector<Endorser> &getEndorsers() const {
    return m_tx_endorsers;
  }
};
} // namespace gruut
#endif
