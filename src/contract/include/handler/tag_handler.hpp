#ifndef TETHYS_SCE_TAG_HANDLER_HPP
#define TETHYS_SCE_TAG_HANDLER_HPP

#include "../data_manager.hpp"
#include "condition_handler.hpp"

namespace tethys::tsce {

class TagHandler {
  tinyxml2::XMLElement *m_info_node;
  tinyxml2::XMLElement *m_update_node;

public:
  TagHandler() = default;

  bool evalue(std::string &tag_str, DataManager &data_manager);

  bool evalue(tinyxml2::XMLElement *doc_node, DataManager &data_manager);

  std::string getName();

  std::vector<std::string> getCword();
};

} // namespace tethys::tsce

#endif
