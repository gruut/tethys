#ifndef GRUUT_PUBLIC_MERGER_UNRESOLVED_BLOCK_POOL_HPP
#define GRUUT_PUBLIC_MERGER_UNRESOLVED_BLOCK_POOL_HPP

#include <atomic>
#include <deque>
#include <time.h>

#include "../../../../lib/gruut-utils/src/time_util.hpp"
#include "../../../../lib/gruut-utils/src/type_converter.hpp"
#include "../../../../lib/log/include/log.hpp"
#include "../config/storage_type.hpp"

#include "../structure/block.hpp"
#include "mem_ledger.hpp"
#include "state_merkle_tree.hpp"

namespace gruut {

struct UnresolvedBlock {
  Block block;
  int32_t prev_vector_idx{-1};
  int32_t ssig_sum{0};
  MemLedger m_mem_ledger;

  UnresolvedBlock() = default;
  UnresolvedBlock(Block &block_, int prev_queue_idx_) : block(block_), prev_vector_idx(prev_queue_idx_) {}
};

struct BlockPosPool {
  int32_t height{0};
  int32_t vector_idx{0};

  BlockPosPool() = default;
  BlockPosPool(int32_t height_, int32_t vector_idx_) : height(height_), vector_idx(vector_idx_) {}
};

class UnresolvedBlockPool {
private:
  std::deque<std::vector<UnresolvedBlock>> m_block_pool; // deque[n] is tree's depth n; vector's blocks are same depth(block height)
  std::recursive_mutex m_push_mutex;

  StateMerkleTree m_us_tree; // user scope state tree
  StateMerkleTree m_cs_tree; // contract scope state tree

  base64_type m_latest_confirmed_id;
  std::atomic<block_height_type> m_latest_confirmed_height;
  timestamp_t m_latest_confirmed_time;
  base64_type m_latest_confirmed_hash;
  base64_type m_latest_confirmed_prev_id;

  base64_type m_head_id;
  std::atomic<block_height_type> m_head_height;
  int m_head_bin_idx;
  int m_head_queue_idx;

public:
  UnresolvedBlockPool();

  inline int32_t size() {
    return m_block_pool.size();
  }

  inline bool empty() {
    return m_block_pool.empty();
  }

  inline void clear() {
    m_block_pool.clear();
  }

  void setPool(const base64_type &last_block_id, block_height_type last_height, timestamp_t last_time, const base64_type &last_hash,
               const base64_type &prev_block_id);

  bool prepareBins(block_height_type t_height);
  ubp_push_result_type push(Block &block, bool is_restore = false);

  bool resolveBlock(Block &block, UnresolvedBlock &resolved_result);

  void restorePool();
  void setupStateTree();

  void processTxResult(UnresolvedBlock &new_UR_block, nlohmann::json &result);
  void moveHead(const std::string &block_id_b64, const block_height_type target_block_height);
  bool queryUserJoin(UnresolvedBlock &UR_block, nlohmann::json &option);
  bool queryUserCert(UnresolvedBlock &UR_block, nlohmann::json &option);
  bool queryIncinerate(UnresolvedBlock &UR_lock, nlohmann::json &option);
  bool queryCreate(UnresolvedBlock &UR_block, nlohmann::json &option);
  bool queryTransfer(UnresolvedBlock &UR_block, nlohmann::json &option);
  bool queryUserScope(UnresolvedBlock &UR_block, nlohmann::json &option);
  bool queryContractScope(UnresolvedBlock &UR_block, nlohmann::json &option);
  bool queryRunQuery(UnresolvedBlock &UR_block, nlohmann::json &option);
  bool queryRunContract(UnresolvedBlock &UR_block, nlohmann::json &option);

  void invalidateCaches();

private:
  void updateTotalNumSSig();

  void backupPool();
  nlohmann::json readBackupIds();
};

} // namespace gruut

#endif // WORKSPACE_UNRESOLVED_BLOCK_POOL_HPP