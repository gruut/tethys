#include "include/condition_manager.hpp"
#include "../../lib/tethys-utils/src/sha256.hpp"

namespace tethys::tsce {

bool tsce::ConditionManager::evalue(tinyxml2::XMLElement *doc_node, DataManager &data_manager) {

  if (doc_node == nullptr)
    return false;

  std::string condition_id = mt::c2s(doc_node->Attribute("id"));
  if (condition_id.empty())
    return true;

  std::vector<uint8_t> hash_kv;

  // TODO : extract keywords

  auto it_map = m_keyword_map.find(condition_id);
  if (it_map != m_keyword_map.end()) { // we know all keywords for this condition
    hash_kv = getHashKV(it_map->second, data_manager);
  } else { // we don't know any keywords!
    std::function<void(tinyxml2::XMLElement *, std::vector<std::string> &)> keyword_search;
    keyword_search = [=, &keyword_search](tinyxml2::XMLElement *doc_node, std::vector<std::string> &keywords) {
      std::string tag_name = mt::c2s(doc_node->Name());

      if (tag_name == "compare") {
        checkAndPush(doc_node, "src", keywords);
        checkAndPush(doc_node, "ref", keywords);
      } else if (tag_name == "age") {
        checkAndPush(doc_node, "after", keywords);
        checkAndPush(doc_node, "before", keywords);
      } else if (tag_name == "var") {
        checkAndPush(doc_node, "id", keywords);
        checkAndPush(doc_node, "name", keywords);
      } else {
        checkAndPush(doc_node, "value", keywords);
      }

      tinyxml2::XMLElement *element_ptr;
      element_ptr = doc_node->FirstChildElement();
      while (element_ptr) {
        keyword_search(element_ptr, keywords);
        element_ptr = element_ptr->NextSiblingElement();
      }
    };

    std::vector<std::string> keyword_list;
    keyword_search(doc_node, keyword_list);
    hash_kv = getHashKV(keyword_list, data_manager);

    m_keyword_map.insert({condition_id, keyword_list});
  }

  bool eval_result = false;

  auto it_cache = m_result_cache.find(condition_id);
  if (it_cache == m_result_cache.end()) {
    eval_result = m_condition_handler.evalue(doc_node, data_manager);
    m_result_cache.insert({condition_id, {eval_result, hash_kv}});
  } else {
    if (hash_kv == it_cache->second.second) { // no value changed, cache is valid
      eval_result = it_cache->second.first;
    } else { // value is changed, cache is invalid -> re-evaluate this condition
      eval_result = m_condition_handler.evalue(doc_node, data_manager);
      m_result_cache.insert({condition_id, {eval_result, hash_kv}});
    }
  }

  return eval_result;
}

std::vector<uint8_t> ConditionManager::getHashKV(std::vector<std::string> &keywords, DataManager &data_manager) {

  std::string kv_string;
  for (auto &each_kw : keywords) {
    kv_string.append(data_manager.get(each_kw).value_or(""));
  }

  return Sha256::hash(kv_string);
}

void ConditionManager::checkAndPush(tinyxml2::XMLElement *doc_node, const std::string &attr_name, std::vector<std::string> &keywords) {
  std::string attr_value = mt::c2s(doc_node->Attribute(attr_name.c_str()));
  if (attr_value.empty())
    return;

  if (attr_value[0] == '$')
    keywords.push_back(attr_value);
}
}