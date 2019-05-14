#ifndef GRUUT_PUBLIC_MERGER_BLOCK_VALIDATOR_HPP
#define GRUUT_PUBLIC_MERGER_BLOCK_VALIDATOR_HPP

#include "../config/storage_type.hpp"
#include "../structure/transaction.hpp"
#include "../structure/block.hpp"

namespace gruut {

std::vector<hash_t> makeStaticMerkleTree(const std::vector<string> &material);
bool calcStateRoot();   // user scope, contract scope의 root를 구할 때 쓰이는 동적 머클 트리

bool verifyTransaction(Transaction &tx, string world, string chain);
bool verifyBlock(Block &block);

bool earlyStage(Block &block);
bool lateStage(Block &block);

} // namespace gruut

#endif