#ifndef GRUUT_PUBLIC_MERGER_BLOCK_VALIDATOR_HPP
#define GRUUT_PUBLIC_MERGER_BLOCK_VALIDATOR_HPP

#include "block.hpp"

namespace gruut {

std::vector<hash_t> makeStaticMerkleTree(std::vector<string> &material) {

  std::vector<hash_t> merkle_tree_vector;
  std::vector<hash_t> sha256_material;

  // 입력으로 들어오는 material 벡터는 해시만 하면 되도록 처리된 상태로 전달되어야 합니다
  for (auto &each_element : material) {
    sha256_material.emplace_back(Sha256::hash(each_element));
  }

  StaticMerkleTree merkle_tree;
  merkle_tree.generate(sha256_material);
  merkle_tree_vector = merkle_tree.getStaticMerkleTree();

  return merkle_tree_vector;
}

// user scope, contract scope의 root를 구할 때 쓰이는 동적 머클 트리
bool calcStateRoot() {
  // 이건 구현하려면 ledger가 필요. 그리고 enterprise에 없던 부분이라서 새로 만들어야 함.
}

bool verifyTransaction(Transaction &tx, string world, string chain) {
  BytesBuilder tx_id_builder;
  tx_id_builder.appendBase<58>(tx.getUserId());
  tx_id_builder.append(world);
  tx_id_builder.append(chain);
  tx_id_builder.appendDec(tx.getTxTime());
  if (!tx.getReceiverId().empty())
    tx_id_builder.appendBase<58>(tx.getReceiverId());
  tx_id_builder.appendDec(tx.getFee());
  tx_id_builder.append(tx.getContractId());
  tx_id_builder.append(TypeConverter::bytesToString(tx.getTxInputCbor()));

  hash_t tx_id = Sha256::hash(tx_id_builder.getBytes());
  if (tx.getTxid() != TypeConverter::encodeBase<58>(tx_id)) {
    return false;
  }

  vector<Endorser> endorsers = tx.getEndorsers();
  for (auto &each_end : endorsers) {
    if (each_end.endorser_signature != SignByEndorser(tx_id)) { // TODO: SignByEndorser가 확정되면 변경해야 함
      return false;
    }
  }

  BytesBuilder user_sig_builder;
  user_sig_builder.append(TypeConverter::bytesToString(tx_id));
  for (auto &each_end : endorsers) {
    user_sig_builder.appendBase<58>(each_end.endorser_id);
    user_sig_builder.append(each_end.endorser_pk);
    user_sig_builder.appendBase<64>(each_end.endorser_signature);
  }
  base64_type user_sig = SignByUser(user_sig_builder.getBytes()); // TODO: SignByUser가 확정되면 변경해야 함
  if (tx.getUserSig() != user_sig) {
    return false;
  }

  return true;
}

bool verifyBlock(Block &block) {

  //
  // TODO: SSig의 퀄리티, 수 검증 추가
  // TODO: block time validation 추가
  //

  BytesBuilder block_id_builder;
  block_id_builder.appendBase<58>(block.getBlockProdId());
  block_id_builder.appendDec(block.getBlockTime());
  block_id_builder.append(block.getWorldId());
  block_id_builder.append(block.getChainId());
  block_id_builder.appendDec(block.getHeight());
  block_id_builder.appendBase<58>(block.getPrevBlockId());

  hash_t block_id = Sha256::hash(block_id_builder.getBytes());
  if (block.getBlockId() != TypeConverter::encodeBase<58>(block_id)) {
    return false;
  }

  vector<hash_t> tx_merkle_tree = makeStaticMerkleTree(block.getTxaggs());
  if (block.getTxRoot() != TypeConverter::encodeBase<64>(tx_merkle_tree.back())) {
    return false;
  }

  BytesBuilder block_hash_builder;
  block_hash_builder.appendBase<58>(block.getBlockId());
  block_hash_builder.appendBase<64>(block.getTxRoot());
  block_hash_builder.appendBase<64>(block.getUserStateRoot());
  block_hash_builder.appendBase<64>(block.getContractStateRoot());

  hash_t block_hash = Sha256::hash(block_hash_builder.getBytes());
  if (block.getBlockHash() != TypeConverter::encodeBase<64>(block_hash)) {
    return false;
  }

  vector<Signature> signers = block.getSigners();
  vector<string> signer_id_sigs;
  signer_id_sigs.clear();
  for (auto &each_signer : signers) {
    string id_sig = each_signer.signer_id + each_signer.signer_sig; // need bytebuilder?
    signer_id_sigs.emplace_back(id_sig);
  }
  vector<hash_t> sg_merkle_tree = makeStaticMerkleTree(signer_id_sigs);
  if (block.getSgRoot() != TypeConverter::encodeBase<64>(sg_merkle_tree.back())) {
    return false;
  }

  //
  // TODO: us_state_root 검증 추가
  // TODO: cs_state_root 검증 추가
  //

  BytesBuilder prod_sig_builder;
  prod_sig_builder.appendDec(block.getBlockPubTime());
  prod_sig_builder.appendBase<64>(block.getBlockHash());
  prod_sig_builder.appendDec(block.getNumTransaction()); // 32bit? 64bit?
  prod_sig_builder.appendDec(block.getNumSigners());     // 32bit? 64bit?
  prod_sig_builder.appendBase<64>(block.getSgRoot());

  string prod_sig = SignByMerger(prod_sig_builder.getBytes()); // TODO: SignByMerger가 확정되면 변경해야 함
  if (block.getBlockProdSig() != prod_sig) {
    return false;
  }
}

bool earlyStage(Block &block) {
  if (!verifyBlock(block))
    return false;

  vector<Transaction> transactions = block.getTransactions();
  for (auto &each_transaction : transactions) {
    if (!verifyTransaction(each_transaction, block.getWorldId(), block.getChainId()))
      return false;
  }

  return true;
}

bool lateStage(Block &block) {
  BytesBuilder block_id_builder;
  block_id_builder.appendBase<58>(block.getBlockProdId());
  block_id_builder.appendDec(block.getBlockTime());
  block_id_builder.append(block.getWorldId());
  block_id_builder.append(block.getChainId());
  block_id_builder.appendDec(block.getHeight());
  block_id_builder.appendBase<58>(block.getPrevBlockId());

  hash_t block_id = Sha256::hash(block_id_builder.getBytes());
  if (block.getBlockId() != TypeConverter::encodeBase<58>(block_id)) {
    return false;
  }

  BytesBuilder signer_sig_builder;
  signer_sig_builder.appendBase<58>(block.getBlockId());
  signer_sig_builder.appendBase<64>(block.getTxRoot());
  signer_sig_builder.appendBase<64>(block.getUserStateRoot());
  signer_sig_builder.appendBase<64>(block.getContractStateRoot());

  hash_t block_info_hash = Sha256::hash(signer_sig_builder.getBytes());
  vector<Signature> signers = block.getSigners();
  for (auto &each_signer : signers) {
    base64_type ssig = SignBySigner(block_info_hash); // TODO: SignBySigner가 확정되면 변경해야 함
    if (each_signer.signer_sig != ssig) {
      return false;
    }
  }

  return true;
}

} // namespace gruut

#endif