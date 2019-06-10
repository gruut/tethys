#ifndef TETHYS_SCE_FEE_HANDLER_HPP
#define TETHYS_SCE_FEE_HANDLER_HPP

#include "config.hpp"
#include "data_manager.hpp"
#include "condition_manager.hpp"

namespace tethys::tsce {

class FeeHandler {
public:
  FeeHandler() = default;

  std::optional<std::pair<int,int>> parseGet(std::vector<std::pair<tinyxml2::XMLElement*,std::string>> &fee_nodes, ConditionManager &condition_manager, DataManager &data_manager) {
    if(fee_nodes.empty()) {
      return std::nullopt;
    }

    int pay_from_user = 0;
    int pay_from_author = 0;

    for(auto &[fee_node, condition_id] : fee_nodes) {

      if(!condition_id.empty() && !condition_manager.getEvalResultById(condition_id))
        continue;

      auto pay_nodes = XmlTool::parseChildrenFromNoIf(fee_node,"pay");

      for(auto &pay_node : pay_nodes) {
        std::string from = mt::c2s(pay_node->Attribute("from"));
        std::string value = mt::c2s(pay_node->Attribute("value"));

        if(from == "user") {
          value = data_manager.eval(value);
          pay_from_user = mt::str2num<int>(value);
        } else {
          pay_from_author = mt::str2num<int>(value);
        }

      }

    }

    // if there is multiple valid fee nodes, the last valid fee node will work

    return std::make_pair(pay_from_user,pay_from_author);
  }

};
}

#endif
