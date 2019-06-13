#ifndef TETHYS_SCE_HANDLER_TIME_HPP
#define TETHYS_SCE_HANDLER_TIME_HPP

#include "base_condition_handler.hpp"

namespace tethys::tsce {

class TimeHandler : public BaseConditionHandler {
public:
  TimeHandler() = default;

  bool evalue(tinyxml2::XMLElement* doc_node, DataManager &data_manager) override;
};

}

#endif
