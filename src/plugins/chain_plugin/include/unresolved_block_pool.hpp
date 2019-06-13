#ifndef TETHYS_PUBLIC_MERGER_UNRESOLVED_BLOCK_POOL_HPP
#define TETHYS_PUBLIC_MERGER_UNRESOLVED_BLOCK_POOL_HPP

#include <atomic>
#include <deque>
#include <map>
#include <memory>
#include <time.h>

#include "../../../../lib/log/include/log.hpp"
#include "../../../../lib/tethys-utils/src/time_util.hpp"
#include "../../../../lib/tethys-utils/src/type_converter.hpp"
#include "../config/storage_type.hpp"

#include "../structure/block.hpp"
#include "mem_ledger.hpp"

namespace tethys {

struct UnresolvedBlock {
  Block block;
  int32_t cur_vec_idx{-1};
  int32_t prev_vec_idx{-1};
  int32_t ssig_sum{0};
  std::map<string, user_ledger_type> user_ledger_list;
  std::map<string, contract_ledger_type> contract_ledger_list;
  std::map<base58_type, user_attribute_type> user_attribute_list;
  std::map<base58_type, user_cert_type> user_cert_list;
  std::map<contract_id_type, contract_type> contract_list;

  UnresolvedBlock() = default;
  UnresolvedBlock(Block &block_, int cur_vec_idx_, int prev_vec_idx_)
      : block(block_), cur_vec_idx(cur_vec_idx_), prev_vec_idx(prev_vec_idx_) {}
};

class UnresolvedBlockPool {
private:
  std::deque<std::vector<UnresolvedBlock>> m_block_pool; // deque[n] is tree's depth n; vector's blocks are same depth(block height)
  std::recursive_mutex m_push_mutex;

  base64_type m_latest_confirmed_id;
  block_height_type m_latest_confirmed_height;
  timestamp_t m_latest_confirmed_time;
  base64_type m_latest_confirmed_hash;
  base64_type m_latest_confirmed_prev_id;

public:
  UnresolvedBlockPool();

  inline int32_t size();
  inline bool empty();
  inline void clear();
  base58_type getLatestConfirmedId();
  block_height_type getLatestConfirmedHeight();
  timestamp_t getLatestConfiremdTime();
  base64_type getLatestConfiremdHash();
  base58_type getLatestConfiremdPrevId();

  void setPool(const base64_type &last_block_id, block_height_type last_height, timestamp_t last_time, const base64_type &last_hash,
               const base64_type &prev_block_id);

  bool prepareDeque(block_height_type t_height);
  ubp_push_result_type pushBlock(Block &block);
  bool resolveBlock(Block &block, UnresolvedBlock &resolved_result, vector<base58_type> &dropped_block_id);

  UnresolvedBlock findBlock(const base58_type &block_id, const block_height_type block_height);
  UnresolvedBlock getBlock(int pool_deq_idx, int pool_vec_idx);
  vector<int> getLine(const base58_type &block_id, const block_height_type block_height);

  void invalidateCaches();

  nlohmann::json getPoolBlockIds();
  string serializeUserLedgerList(const UnresolvedBlock &unresolved_block);
  string serializeContractLedgerList(const UnresolvedBlock &unresolved_block);
  string serializeUserAttributeList(const UnresolvedBlock &unresolved_block);
  string serializeUserCertList(const UnresolvedBlock &unresolved_block);
  string serializeContractList(const UnresolvedBlock &unresolved_block);
  void restorePool();

private:
  void updateTotalNumSSig();
};

} // namespace tethys

#endif // WORKSPACE_UNRESOLVED_BLOCK_POOL_HPP