#pragma once

#include <rttr/registration>
#include <string>

using namespace std;

struct Transaction {
  int tsidx;             // transaction scope index
  string tx_id;          // transaction’s identifier
  uint64_t tx_time;      // transaction’s time
  string tx_seed;        // seed included in transaction
  string tx_contract_id; // contract’s identifier for this transaction
  int tx_fee_author;     // transaction fee by author
  int tx_fee_user;       // transaction fee by user
  string tx_user;        // transaction producer’s identifier
  string tx_user_pk;     // transaction producer’s public key
  string tx_receiver;    // receiver’s identifier
  string tx_input;       // contract’s input
  string tx_user_sig;    // transaction signature
  string tx_agg_cbor;    // serialized tx tx_agg by CBOR
  string block_id;       // block’s identifier including this transaction
  int tx_pos;            // position on Merkle tree
  string tx_output;      // contract’s output

  RTTR_ENABLE()
};

RTTR_REGISTRATION {
  rttr::registration::class_<Transaction>("Transaction")
      .property("tsidx", &Transaction::tsidx)
      .property("tx_id", &Transaction::tx_id)
      .property("tx_time", &Transaction::tx_time)
      .property("tx_seed", &Transaction::tx_seed)
      .property("tx_contract_id", &Transaction::tx_contract_id)
      .property("tx_fee_author", &Transaction::tx_fee_author)
      .property("tx_fee_user", &Transaction::tx_fee_user)
      .property("tx_user", &Transaction::tx_user)
      .property("tx_user_pk", &Transaction::tx_user_pk)
      .property("tx_receiver", &Transaction::tx_receiver)
      .property("tx_input", &Transaction::tx_input)
      .property("tx_user_sig", &Transaction::tx_user_sig)
      .property("tx_agg_cbor", &Transaction::tx_agg_cbor)
      .property("block_id", &Transaction::block_id)
      .property("tx_pos", &Transaction::tx_pos)
      .property("tx_output", &Transaction::tx_output);
}
