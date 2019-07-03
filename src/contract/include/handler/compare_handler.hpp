#ifndef TETHYS_SCE_HANDLER_COMPARE_HPP
#define TETHYS_SCE_HANDLER_COMPARE_HPP

#include "base_condition_handler.hpp"

namespace tethys::tsce {

enum class CompareType : int { EQ, NE, GE, LE, GT, LT, AGE, ALE, AGT, ALT, UNKNOWN };

class CompareHandler : public BaseConditionHandler {
public:
  CompareHandler() = default;

  bool evalue(tinyxml2::XMLElement *doc_node, DataManager &data_manager) override;

private:
  CompareType getCompareType(std::string_view type_str);
};

} // namespace tethys::tsce

#endif
