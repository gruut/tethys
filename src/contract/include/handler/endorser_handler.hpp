#ifndef TETHYS_SCE_HANDLER_ENDORSER_HPP
#define TETHYS_SCE_HANDLER_ENDORSER_HPP

#include "base_condition_handler.hpp"

namespace tethys::tsce {

  class EndorserHandler : public BaseConditionHandler {
  public:
    EndorserHandler() = default;

    bool evalue(tinyxml2::XMLElement *doc_node, DataManager &data_manager) override;

  };

}

#endif
