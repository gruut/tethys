#ifndef GRUUT_PUBLIC_MERGER_UNRESOLVED_BLOCK_POOL_HPP
#define GRUUT_PUBLIC_MERGER_UNRESOLVED_BLOCK_POOL_HPP

#include <atomic>
#include <deque>
#include <time.h>
#include <memory>
#include <map>

#include "../../../../lib/gruut-utils/src/time_util.hpp"
#include "../../../../lib/gruut-utils/src/type_converter.hpp"
#include "../../../../lib/log/include/log.hpp"
#include "../config/storage_type.hpp"

#include "../structure/block.hpp"
#include "mem_ledger.hpp"

namespace gruut {

struct UnresolvedBlock {
  Block block;
  int32_t prev_vector_idx{-1};
  int32_t ssig_sum{0};
  std::map<string, LedgerRecord> user_ledger;
  std::map<string, LedgerRecord> contract_ledger;

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

  StateTree m_us_tree; // user scope state tree
  StateTree m_cs_tree; // contract scope state tree

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

  base58_type getLatestConfirmedId() {
    return m_latest_confirmed_id;
  }

  block_height_type getLatestConfirmedHeight() {
    return m_latest_confirmed_height;
  }

  timestamp_t getLatestConfiremdTime() {
    return m_latest_confirmed_time;
  }

  base64_type getLatestConfiremdHash() {
    return m_latest_confirmed_hash;
  }

  base58_type getLatestConfiremdPrevId() {
    return m_latest_confirmed_prev_id;
  }

  base58_type getCurrentHeadId() {
    return m_head_id;
  }

  block_height_type getCurrentHeadHeight() {
    return m_head_height;
  }

  void setPool(const base64_type &last_block_id, block_height_type last_height, timestamp_t last_time, const base64_type &last_hash,
               const base64_type &prev_block_id);

  bool prepareBins(block_height_type t_height);
  ubp_push_result_type push(Block &block, bool is_restore = false);
  bool resolveBlock(Block &block, UnresolvedBlock &resolved_result);

  void restorePool();
  void setupStateTree();
  bytes getUserStateRoot();
  bytes getContractStateRoot();

  UnresolvedBlock findBlock(const base58_type &block_id, const block_height_type block_height);
  void moveHead(const std::string &block_id_b64, const block_height_type target_block_height);
  void invalidateCaches();

private:
  void updateTotalNumSSig();

  void backupPool();
  nlohmann::json readBackupIds();
};

} // namespace gruut

#endif // WORKSPACE_UNRESOLVED_BLOCK_POOL_HPP