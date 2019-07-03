#ifndef TETHYS_SCE_HANDLER_VAR_HPP
#define TETHYS_SCE_HANDLER_VAR_HPP

#include "base_condition_handler.hpp"

namespace tethys::tsce {

class VarHandler : public BaseConditionHandler {
public:
  VarHandler() = default;

  bool evalue(tinyxml2::XMLElement *doc_node, DataManager &data_manager) override;
};
} // namespace tethys::tsce

#endif
