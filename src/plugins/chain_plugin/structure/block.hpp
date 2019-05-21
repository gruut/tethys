#ifndef GRUUT_PUBLIC_MERGER_BLOCK_HPP
#define GRUUT_PUBLIC_MERGER_BLOCK_HPP

#include "../config/storage_type.hpp"
#include "../include/static_merkle_tree.hpp"
#include "certificate.hpp"
#include "signature.hpp"
#include "transaction.hpp"

#include "../../../../lib/json/include/json.hpp"

using namespace std;

namespace gruut {

class Block {
private:
  timestamp_t m_block_pub_time; // msg.btime
  base58_type m_block_id;
  timestamp_t m_block_time; // msg.block.time
  alphanumeric_type m_world_id;
  alphanumeric_type m_chain_id;
  block_height_type m_block_height;

private:
  base58_type m_block_prev_id;
  base64_type m_block_prev_sig; // msg(-1).producer.sig
  base64_type m_block_hash;

  std::vector<txagg_cbor_b64> m_txaggs; // Tx의 Json을 CBOR로 처리하고 그 데이터를 b64인코딩한 결과 vector
  std::vector<Transaction> m_transactions; // RDB 블록/트랜잭션 정보에 넣어야하기 때문에 유지. tx_root 계산할 때에도 사용

  vector<hash_t> m_tx_merkle_tree;
  base64_type m_tx_root;
  base64_type m_us_state_root;
  base64_type m_cs_state_root;
  base64_type m_sg_root;

  std::vector<Signature> m_signers;      // <signer_id, signer_sig> 형태
  std::vector<Certificate> m_user_certs; // <cert_id, cert_content> 형태

  Signature m_block_prod_info;

  string m_block_certificate;

public:
  Block() = default;

  bool operator==(Block &other) const {
    return (m_block_height == other.getHeight() && m_block_hash == other.getBlockHash());
  }

  bool initialize(const nlohmann::json &msg_block) {

    if (msg_block.empty())
      return false;

    m_block_pub_time = static_cast<gruut::timestamp_t>(stoll(json::get<string>(msg_block, "btime").value()));

    m_block_id = json::get<string>(msg_block["block"], "id").value();
    m_block_time = static_cast<gruut::timestamp_t>(stoll(json::get<string>(msg_block["block"], "time").value()));
    m_world_id = json::get<string>(msg_block["block"], "world").value();
    m_chain_id = json::get<string>(msg_block["block"], "chain").value();
    m_block_height = stoi(json::get<string>(msg_block["block"], "height").value());
    m_block_prev_id = json::get<string>(msg_block["block"], "previd").value();
    m_block_prev_sig = json::get<string>(msg_block["block"], "link").value();
    m_block_hash = json::get<string>(msg_block["block"], "hash").value();

    setTxaggs(msg_block["tx"]);
    setTransaction(m_txaggs); // txagg에서 트랜잭션 정보를 추출

    m_tx_root = json::get<string>(msg_block["state"], "txroot").value();
    m_us_state_root = json::get<string>(msg_block["state"], "usroot").value();
    m_cs_state_root = json::get<string>(msg_block["state"], "csroot").value();
    m_sg_root = json::get<string>(msg_block["state"], "sgroot").value();

    if (!setSigners(msg_block["signer"]))
      return false;
    setUserCerts(msg_block["certificate"]);

    m_block_prod_info.signer_id = json::get<string>(msg_block["producer"], "id").value();
    m_block_prod_info.signer_sig = json::get<string>(msg_block["producer"], "sig").value();

    m_block_certificate = msg_block["certificate"].dump(); // TODO: 위의 UserCerts를 전부 Stringfy한 값. 확인 필요

    return true;
  }

  void setBlockId(const base58_type &blockId) {
    m_block_id = blockId;
  }

  void setHeight(block_height_type blockHeight) {
    m_block_height = blockHeight;
  }

  void setBlockHash(const base64_type &blockHash) {
    m_block_hash = blockHash;
  }

  void setBlockTime(timestamp_t blockTime) {
    m_block_time = blockTime;
  }

  void setBlockPubTime(timestamp_t blockPubTime) {
    m_block_pub_time = blockPubTime;
  }

  void setBlockPrevId(const base58_type &blockPrevId) {
    m_block_prev_id = blockPrevId;
  }

  void setBlockPrevSig(const base64_type &blockPrevSig) {
    m_block_prev_sig = blockPrevSig;
  }

  void setProducerId(const base58_type &blockProdId) {
    m_block_prod_info.signer_id = blockProdId;
  }

  void setProducerSig(const base64_type &blockProdSig) {
    m_block_prod_info.signer_sig = blockProdSig;
  }

  void setTxRoot(const base64_type &txRoot) {
    m_tx_root = txRoot;
  }

  void setUsStateRoot(const base64_type &usStateRoot) {
    m_us_state_root = usStateRoot;
  }

  void setCsStateRoot(const base64_type &csStateRoot) {
    m_cs_state_root = csStateRoot;
  }

  void setSgRoot(const base64_type &sgRoot) {
    m_sg_root = sgRoot;
  }

  bool setTxaggs(const nlohmann::json &txs_json) {
    if (!txs_json.is_array()) {
      return false;
    }

    m_txaggs.clear();
    for (auto &each_tx_json : txs_json) {
      m_txaggs.emplace_back(each_tx_json);
    }
    return true;
  }

  bool setTransaction(std::vector<txagg_cbor_b64> &txagg) {
    m_transactions.clear();
    for (auto &each_txagg : txagg) {
      nlohmann::json each_txs_json;
      each_txs_json = nlohmann::json::from_cbor(TypeConverter::decodeBase<64>(each_txagg));

      Transaction each_tx;
      block_info_type block_info(each_txagg, getBlockId());
      each_tx.inputMsgTxAgg(each_txs_json, block_info);
      m_transactions.emplace_back(each_tx);
    }
    return true;
  }

  bool setSigners(const std::vector<Signature> &signers) {
    if (signers.empty())
      return false;
    m_signers = signers;
    return true;
  }

  bool setSigners(const nlohmann::json &signers) {
    if (!signers.is_array())
      return false;

    m_signers.clear();
    for (auto &each_signer : signers) {
      m_signers.emplace_back(json::get<string>(each_signer, "id").value(), json::get<string>(each_signer, "sig").value());
    }
    return true;
  }

  bool setUserCerts(const nlohmann::json &certificates) {
    m_user_certs.clear();
    for (auto &each_cert : certificates) {
      Certificate tmp;

      auto id = json::get<string>(each_cert, "id");
      if(!id.has_value()) {
        continue;
      }

      tmp.cert_id = id.value();

      string cert_content = "";
      nlohmann::json cert = each_cert["cert"]; // 예외처리 필요?
      for (int i = 0; i < cert.size(); ++i) {
        cert_content += cert[i].get<string>();
        cert_content += "\n";
      }
      tmp.cert_content = cert_content;

      m_user_certs.emplace_back(tmp);
    }
    return true;
  }

  timestamp_t getBlockPubTime() const {
    return m_block_pub_time;
  }

  base58_type getBlockId() const {
    return m_block_id;
  }

  timestamp_t getBlockTime() const {
    return m_block_time;
  }

  alphanumeric_type getWorldId() const {
    return m_world_id;
  }

  alphanumeric_type getChainId() const {
    return m_chain_id;
  }

  block_height_type getHeight() const {
    return m_block_height;
  }

  base58_type getPrevBlockId() const {
    return m_block_prev_id;
  }

  base64_type getPrevBlockSig() const {
    return m_block_prev_sig;
  }

  base64_type getBlockHash() const {
    return m_block_hash;
  }

  std::vector<txagg_cbor_b64> getTxaggs() const {
    std::vector<txagg_cbor_b64> ret_txaggs;
    for (auto &each_tx : m_txaggs) {
      ret_txaggs.emplace_back(each_tx);
    }
    return ret_txaggs;
  }

  std::vector<Transaction> &getTransactions() {
    return m_transactions;
  }

  int32_t getNumTransaction() const {
    return m_transactions.size();
  }

  base64_type getTxRoot() const {
    return m_tx_root;
  }

  base64_type getUserStateRoot() const {
    return m_us_state_root;
  }

  base64_type getContractStateRoot() const {
    return m_cs_state_root;
  }

  base64_type getSgRoot() const {
    return m_sg_root;
  }

  std::vector<Signature> getSigners() const {
    return m_signers;
  }

  int32_t getNumSigners() const {
    return m_signers.size();
  }

  std::vector<Certificate> getUserCerts() const {
    return m_user_certs;
  }

  base58_type getBlockProdId() const {
    return m_block_prod_info.signer_id;
  }

  base64_type getBlockProdSig() const {
    return m_block_prod_info.signer_sig;
  }

  string getBlockCert() const {
    return m_block_certificate;
  }
};
} // namespace gruut
#endif
