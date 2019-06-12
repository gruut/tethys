#include "include/handler/base_condition_handler.hpp"
#include "include/config.hpp"
#include "include/datamap.hpp"

namespace tethys::tsce {
  optional<tethys::tsce::EvalRuleType> tethys::tsce::BaseConditionHandler::getEvalRule(std::string_view eval_str) {
    if (eval_str.empty())
      return std::nullopt;

    return (mt::toLower(eval_str) == "or") ? EvalRuleType::OR : EvalRuleType::AND;
  }

  PrimaryConditionType BaseConditionHandler::getPrimaryConditionType(std::string_view condition_tag) {
    if (condition_tag.empty())
      return PrimaryConditionType::UNKNOWN;

    std::string cond_tag_lower = mt::toLower(condition_tag);

    static std::map<std::string, PrimaryConditionType> tag_to_type_map = {
            {"condition",   PrimaryConditionType::ROOT},
            {"update",      PrimaryConditionType::ROOT},
            {"if",          PrimaryConditionType::IF},
            {"nif",         PrimaryConditionType::NIF},
            {"compare",     PrimaryConditionType::COMPARE},
            {"signature",   PrimaryConditionType::SIGNATURE},
            {"certificate", PrimaryConditionType::CERTIFICATE},
            {"time",        PrimaryConditionType::TIME},
            {"endorser",    PrimaryConditionType::ENDORSER},
            {"receiver",    PrimaryConditionType::RECEIVER},
            {"user",        PrimaryConditionType::USER},
            {"var",         PrimaryConditionType::VAR}
    };

    auto it_map = tag_to_type_map.find(cond_tag_lower);
    if (it_map == tag_to_type_map.end()) {
      return PrimaryConditionType::UNKNOWN;
    }

    return it_map->second;
  }

  SecondaryConditionType BaseConditionHandler::getSecondaryConditionType(std::string_view condition_tag) {
    if (condition_tag.empty())
      return SecondaryConditionType::UNKNOWN;

    std::string cond_tag_lower = mt::toLower(condition_tag);

    static std::map<std::string, SecondaryConditionType> tag_to_type_map = {
            {"if",       SecondaryConditionType::IF},
            {"nif",      SecondaryConditionType::NIF},
            {"age",      SecondaryConditionType::AGE},
            {"gender",   SecondaryConditionType::GENDER},
            {"service",  SecondaryConditionType::SERVICE},
            {"id",       SecondaryConditionType::ID},
            {"location", SecondaryConditionType::LOCATION},
            {"endorser", SecondaryConditionType::ENDORSER},
            {"user",     SecondaryConditionType::USER},
            {"receiver", SecondaryConditionType::RECEIVER}
    };

    auto it_map = tag_to_type_map.find(cond_tag_lower);
    if (it_map == tag_to_type_map.end()) {
      return SecondaryConditionType::UNKNOWN;
    }

    return it_map->second;
  }
}
