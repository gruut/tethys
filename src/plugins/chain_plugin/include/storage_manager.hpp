#ifndef GRUUT_PUBLIC_MERGER_STORAGE_MANAGER_HPP
#define GRUUT_PUBLIC_MERGER_STORAGE_MANAGER_HPP

#include "kv_store.hpp"
#include "mem_ledger.hpp"

namespace gruut {
//
//class StorageManager : public TemplateSingleton<StorageManager> {
//private:
//  KvController *m_storage;
//  mem_ledger_t m_mem_ledger;
//
//public:
//  StorageManager() {
//    logger::getLogger("LAYS");
//    m_storage = KvController::getInstance();
//  }
//
//  bool saveLedger(mem_ledger_t &mem_ledger, const std::string &prefix = "") {
//    m_mem_ledger.append(mem_ledger, prefix);
//    return true;
//  }
//
//  bool saveLedger(const std::string &key, const std::string &value, const std::string &block_id_b64 = "") {
//
//    CLOG(INFO, "LAYS") << "new record : " << key << "[" << block_id_b64 << "]";
//
//    if (block_id_b64.empty())
//      return m_storage->saveLedger(key, value);
//
//    m_mem_ledger.push(key, value, block_id_b64);
//
//    return true;
//  }
//
//  template <typename T = std::string, typename V = block_layer_t>
//  std::string readLedgerByKey(T &&key, V &&block_layer = {}) {
//    std::string ret_val;
//
//    if (block_layer.empty()) {
//      for (auto &each_block_id_b64 : m_block_layer) { // reverse_order
//        if (m_mem_ledger.getVal(key, each_block_id_b64, ret_val)) {
//          break;
//        }
//      }
//    } else {
//      for (auto &each_block_id_b64 : block_layer) { // reverse_order
//        if (m_mem_ledger.getVal(key, each_block_id_b64, ret_val)) {
//          break;
//        }
//      }
//    }
//
//    if (ret_val.empty())
//      ret_val = m_storage->readLedgerByKey(key);
//
//    return ret_val;
//  }
//
//  void flushLedger() {
//    m_storage->flushLedger();
//  }
//
//  void clearLedger() {
//    m_mem_ledger.clear();
//  }
//
//  template <typename T = std::string>
//  void moveToDiskLedger(T &&block_id_b64) { // 이 함수 바꿔야함
//    auto kv_vector = m_mem_ledger.getKV(block_id_b64);
//
//    for (auto &each_record : kv_vector) {
//      m_storage->saveLedger(each_record.key, each_record.value);
//    }
//
//    m_storage->flushLedger();
//    dropLedger(block_id_b64);
//  }
//
//  template <typename T = std::string>
//  void dropLedger(T &&block_id_b64) {
//    m_mem_ledger.dropKV(block_id_b64);
//  }
//};

} // namespace gruut

#endif // WORKSPACE_STORAGE_MANAGER_HPP
