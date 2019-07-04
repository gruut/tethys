#pragma once

#include "../../../../lib/tinyxml/include/tinyxml2.h"
#include "../data_manager.hpp"

namespace tethys::tsce {

class BaseConditionHandler {
public:
  BaseConditionHandler() = default;

  virtual bool evalue(tinyxml2::XMLElement *doc_node, DataManager &data_manager) = 0;

protected:
  std::optional<EvalRuleType> getEvalRule(std::string_view eval_str);

  PrimaryConditionType getPrimaryConditionType(std::string_view condition_tag);

  SecondaryConditionType getSecondaryConditionType(std::string_view condition_tag);
};

} // namespace tethys::tsce
