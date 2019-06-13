#ifndef TETHYS_SCE_CONDITION_MANAGER_HPP
#define TETHYS_SCE_CONDITION_MANAGER_HPP

#include <string>
#include "handler/condition_handler.hpp"

namespace tethys::tsce {

  class ConditionManager {

  public:
    ConditionManager() = default;

    bool evalue(tinyxml2::XMLElement *doc_node, DataManager &data_manager);

    template<typename S = std::string>
    bool getEvalResultById(S &&id) {

      if (id.empty())
        return true;

      std::string condition_id = id;
      bool is_neg = false;

      if (id[0] == '~') {
        is_neg = true;
        condition_id = id.substr(1);
      }

      if (condition_id.empty()) {
        return true;
      }

      auto it_cache = m_result_cache.find(condition_id);
      if (it_cache == m_result_cache.end()) // no cache
        return true;

      return (is_neg == !it_cache->second.first); // is_neg ? !it_cache->second.first : it_cache->second.first
    }

  private:
    std::vector<uint8_t> getHashKV(std::vector<std::string> &keywords, DataManager &data_manager);

    void checkAndPush(tinyxml2::XMLElement *doc_node, const std::string &attr_name, std::vector<std::string> &keywords);

    ConditionHandler m_condition_handler;
    std::map<std::string, std::pair<bool, std::vector<uint8_t>>> m_result_cache; // id, {result, < keywords >}
    std::map<std::string, std::vector<std::string>> m_keyword_map;
  };

}

#endif
