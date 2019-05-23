#ifndef GRUUT_PUBLIC_MERGER_MEM_LEDGER_HPP
#define GRUUT_PUBLIC_MERGER_MEM_LEDGER_HPP

#include "../../../../lib/gruut-utils/src/bytes_builder.hpp"
#include "../../../../lib/log/include/log.hpp"
#include "../config/storage_type.hpp"

#include <list>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

namespace gruut {

struct LedgerRecord {
  LedgerType which_scope; // user scope or contract scope
  string var_name;
  string var_val;
  int var_type;
  string var_owner; // user scope의 경우 uid, contract scope의 경우 cid
  timestamp_t up_time;
  block_height_type up_block;
  string tag;      // user scope only
  string var_info; // contract scope only
  hash_t pid;

  LedgerRecord(LedgerType what_, string var_name_, string var_val_, int var_type_, string var_owner_, timestamp_t up_time_,
               block_height_type up_block_, string tag_info_)
      : which_scope(what_), var_name(var_name_), var_val(var_val_), var_type(var_type_), var_owner(var_owner_), up_time(up_time_),
        up_block(up_block_) {
    if (which_scope == LedgerType::USERSCOPE) {
      tag = tag_info_;

      BytesBuilder bytes_builder;
      bytes_builder.append(var_name);
      bytes_builder.append(var_type);
      bytes_builder.appendBase<58>(var_owner);
      bytes_builder.append(tag);
      pid = Sha256::hash(bytes_builder.getBytes());
    } else if (which_scope == LedgerType::CONTRACTSCOPE) {
      var_info = tag_info_;

      BytesBuilder bytes_builder;
      bytes_builder.append(var_name);
      bytes_builder.append(var_type);
      bytes_builder.append(var_owner);
      bytes_builder.append(var_info);
      pid = Sha256::hash(bytes_builder.getBytes());
    }
  };

  class MemLedger {
  private:
    std::list<LedgerRecord> m_ledger;
    std::mutex m_active_mutex;

  public:
    MemLedger() {}

    bool addUserScope(string var_name, string var_val, string var_type, string var_owner, timestamp_t up_time, block_height_type up_block,
                      string tag) {
      std::lock_guard<std::mutex> lock(m_active_mutex);
      logger::INFO("New UserScope MemLedger Created");

      // TODO: 현 위치에서 RDB까지 체크하면서 값의 갱신 결과 계산

      m_ledger.emplace_back(LedgerType::USERSCOPE, var_name, var_val, var_type, var_owner, up_time, up_block, tag);

      return true;
    }

    bool addContractScope(string var_name, string var_val, string var_type, string var_owner, timestamp_t up_time,
                          block_height_type up_block, string var_info) {
      std::lock_guard<std::mutex> lock(m_active_mutex);
      logger::INFO("New ContractScope MemLedger Created");

      // TODO: 현 위치에서 RDB까지 체크하면서 값의 갱신 결과 계산

      m_ledger.emplace_back(LedgerType::CONTRACTSCOPE, var_name, var_val, var_type, var_owner, up_time, up_block, var_info);

      return true;
    }
  };
} // namespace gruut

#endif
