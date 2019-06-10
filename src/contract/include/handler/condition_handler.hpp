#ifndef TETHYS_SCE_CONDITION_HANDLER_HPP
#define TETHYS_SCE_CONDITION_HANDLER_HPP

#include "base_condition_handler.hpp"

namespace tethys::tsce {

  class ConditionHandler : BaseConditionHandler {

  public:
    ConditionHandler() = default;

    bool evalue(tinyxml2::XMLElement *doc_node, DataManager &data_manager) override;

  };

}

#endif
