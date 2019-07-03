#ifndef TETHYS_SCE_ELEMENT_PARSER_HPP
#define TETHYS_SCE_ELEMENT_PARSER_HPP

#include "xml_tool.hpp"

namespace tethys::tsce {

class ElementParser {
public:
  ElementParser() = default;

  bool setContract(std::string &xml_doc);

  tinyxml2::XMLElement *getNode(std::string_view node_name);

  std::vector<std::pair<tinyxml2::XMLElement *, std::string>> &getNodes(std::string_view node_name);

private:
  tinyxml2::XMLDocument m_contract_doc;
  tinyxml2::XMLElement *m_contract;

  tinyxml2::XMLElement *m_head;
  tinyxml2::XMLElement *m_body;
  tinyxml2::XMLElement *m_input;

  std::vector<std::pair<tinyxml2::XMLElement *, std::string>> m_gets;
  std::vector<std::pair<tinyxml2::XMLElement *, std::string>> m_sets;
  std::vector<std::pair<tinyxml2::XMLElement *, std::string>> m_conditions;
  std::vector<std::pair<tinyxml2::XMLElement *, std::string>> m_oracles;
  std::vector<std::pair<tinyxml2::XMLElement *, std::string>> m_displays;
  std::vector<std::pair<tinyxml2::XMLElement *, std::string>> m_calls;
  std::vector<std::pair<tinyxml2::XMLElement *, std::string>> m_fees;
  std::vector<std::pair<tinyxml2::XMLElement *, std::string>> m_scripts;

  std::vector<std::pair<tinyxml2::XMLElement *, std::string>> m_multi_dummy;
};

} // namespace tethys::tsce

#endif
