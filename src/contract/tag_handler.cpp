#include "include/config.hpp"
#include <algorithm>
#include <iostream>
#include <map>

#include "include/handler/tag_handler.hpp"
#include "include/xml_tool.hpp"

namespace tethys::tsce {
  bool TagHandler::evalue(std::string &tag_str, DataManager &data_manager) {

    tinyxml2::XMLDocument tag_doc;

    tag_doc.Parse(tag_str.c_str());

    if (tag_doc.Error())
      return false;

    tinyxml2::XMLElement *tag_element = tag_doc.RootElement();

    return evalue(tag_element, data_manager);
  }

  bool tsce::TagHandler::evalue(tinyxml2::XMLElement *doc_node, DataManager &data_manager) {

    if (doc_node == nullptr)
      return false;

    m_info_node = doc_node->FirstChildElement("info");
    m_update_node = doc_node->FirstChildElement("update");
    ConditionHandler condition_handler;

    return condition_handler.evalue(m_update_node, data_manager);
  }

  std::string TagHandler::getName() {
    return mt::c2s(m_info_node->Name());
  }

  std::vector<std::string> TagHandler::getCword() {
    std::vector<std::string> ret_vec;
    auto cword_node = XmlTool::parseChildrenFromNoIf(m_info_node, "cword");
    for (auto &each_node : cword_node) {
      ret_vec.emplace_back(mt::c2s(each_node->GetText()));
    }

    return ret_vec;
  }
}
