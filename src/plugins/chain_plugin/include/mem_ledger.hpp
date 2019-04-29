#ifndef GRUUT_PUBLIC_MERGER_MEM_LEDGER_HPP
#define GRUUT_PUBLIC_MERGER_MEM_LEDGER_HPP

#include "../config/storage_type.hpp"
#include "../../../../lib/gruut-utils/src/bytes_builder.hpp"

#include <list>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

namespace gruut {

struct LedgerRecord {
  bool which_scope; // user scope와 contract scope를 구분하기 위한 변수. LedgerType::
  std::string var_name;
  std::string var_val;
  std::string var_type;
  std::string var_owner; // user scope의 경우 uid, contract scope의 경우 cid
  timestamp_t up_time;
  block_height_type up_block; // user scope only
  std::string tag;      // user scope only
  std::string var_info;       // contract scope only
  hash_t pid;

  LedgerRecord(std::string var_name_, std::string var_val_, std::string var_type_, std::string var_owner_, timestamp_t up_time_,
               block_height_type up_block_, std::string tag_)
      : var_name(var_name_), var_val(var_val_), var_type(var_type_), var_owner(var_owner_), up_time(up_time_), up_block(up_block_),
        tag(tag_) {
    //    which_scope = LedgerType::USERSCOPE;
    BytesBuilder bytes_builder;
    bytes_builder.append(var_name);
    bytes_builder.append(var_type);
    bytes_builder.appendBase<58>(var_owner);
    bytes_builder.append(tag);
    pid = Sha256::hash(bytes_builder.getBytes());
  }

  LedgerRecord(std::string var_name_, std::string var_val_, std::string var_type_, std::string var_owner_, timestamp_t up_time_,
               std::string var_info_)
      : var_name(var_name_), var_val(var_val_), var_type(var_type_), var_owner(var_owner_), up_time(up_time_), var_info(var_info_) {
    //    which_scope = LedgerType::CONTRACTSCOPE;
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
  MemLedger() {
    el::Loggers::getLogger("MEML");
  }

  bool addUserScope(std::string var_name, std::string var_val, std::string var_type, std::string var_owner, timestamp_t up_time,
                    block_height_type up_block, std::string tag) {
    std::lock_guard<std::mutex> lock(m_active_mutex);

    // TODO: 현 위치에서 RDB까지 체크하면서 값의 갱신 결과 계산

    m_ledger.emplace_back(var_name, var_val, var_type, var_owner, up_time, up_block, tag);

    return true;
  }

  bool addContractScope(std::string var_name, std::string var_val, std::string var_type, std::string var_owner, timestamp_t up_time,
                        std::string var_info) {
    std::lock_guard<std::mutex> lock(m_active_mutex);
    m_ledger.emplace_back(var_name, var_val, var_type, var_owner, up_time, var_info);

    return true;
  }
};
} // namespace gruut

#endif
