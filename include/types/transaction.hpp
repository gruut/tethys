#pragma once

#include <string>
#include <vector>

using namespace std;

// TODO: Remove the code duplication between TransactionMessage and Transaction.
struct TransactionMessage {
  string txid;
  string world;
  string chain;
  string time;

  string cid;
  string receiver;
  string fee;
  vector<uint8_t> input;

  string user_id;
  string user_pk;
  string user_sig;

  string endorser_id;
  string endorser_pk;
  string endorser_sig;
};

struct Transaction {
  int tsidx;             // transaction scope index, Primary key
  string tx_id;          // transaction’s identifier
  uint64_t tx_time;      // transaction’s time
  string tx_contract_id; // contract’s identifier for this transaction
  int tx_fee_author;     // transaction fee by author
  int tx_fee_user;       // transaction fee by user
  string tx_user;        // transaction producer’s identifier
  string tx_user_pk;     // transaction producer’s public key
  string tx_user_sig;    // transaction signature
  string tx_receiver;    // receiver’s identifier
  string tx_input;       // contract’s input
  string tx_agg_cbor;    // serialized tx tx_agg by CBOR
  string block_id;       // block’s identifier including this transaction
  int tx_pos;            // position on Merkle tree
  string tx_output;      // contract’s output
};
