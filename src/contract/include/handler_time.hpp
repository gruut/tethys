#ifndef TETHYS_SCE_HANDLER_TIME_HPP
#define TETHYS_SCE_HANDLER_TIME_HPP

#include "config.hpp"
#include "base_condition_handler.hpp"

namespace tethys::tsce {

class TimeHandler : public BaseConditionHandler {
public:
  TimeHandler() = default;

  bool evalue(tinyxml2::XMLElement* doc_node, DataManager &data_manager) override {

    if(doc_node == nullptr)
      return false;

    auto timestamp = data_manager.evalOpt("$time");
    if(!timestamp)
      return false;

    uint64_t base_timestamp = mt::str2num<uint64_t>(timestamp.value()) * 1000;

    auto after_node = doc_node->FirstChildElement("after");
    auto before_node = doc_node->FirstChildElement("before");

    std::string time_after;
    std::string time_before;

    if(after_node != nullptr)
      time_after = mt::c2s(after_node->GetText());

    if(before_node != nullptr)
      time_before = mt::c2s(before_node->GetText());

    mt::trim(time_after);
    mt::trim(time_before);

    if(time_after.empty() && time_before.empty())
        return false;

    if(time_before.empty())
        return (mt::timestr2timestamp(time_after) < base_timestamp);

    if(time_after.empty())
        return (mt::timestr2timestamp(time_before) > base_timestamp);

    return (mt::timestr2timestamp(time_after) < base_timestamp && base_timestamp < mt::timestr2timestamp(time_before));

  }
};

}

#endif
