#ifndef TETHYS_SCE_SET_HANDLER_HPP
#define TETHYS_SCE_SET_HANDLER_HPP

#include "../data_manager.hpp"
#include "../condition_manager.hpp"

namespace tethys::tsce {

  enum class SetType : int {
    USER_JOIN,
    USER_CERT,
    V_CREATE,
    V_INCINERATE,
    V_TRANSFER,
    SCOPE_USER,
    SCOPE_CONTRACT,
    CONTRACT_NEW,
    CONTRACT_DISABLE,
    TRADE_ITEM,
    TRADE_V,
    RUN_QUERY,
    RUN_CONTRACT,
    NONE
  };

// clang-format off
  const std::map<std::string, SetType> SET_TYPE_MAP{
          {"user.cert",        SetType::USER_JOIN},
          {"user.cert",        SetType::USER_CERT},
          {"v.create",         SetType::V_CREATE},
          {"v.incinerate",     SetType::V_INCINERATE},
          {"v.transfer",       SetType::V_TRANSFER},
          {"scope.user",       SetType::SCOPE_USER},
          {"scope.contract",   SetType::SCOPE_CONTRACT},
          {"contract.new",     SetType::CONTRACT_NEW},
          {"contract.disable", SetType::CONTRACT_DISABLE},
          {"trade.item",       SetType::TRADE_ITEM},
          {"trade.v",          SetType::TRADE_V},
          {"run.query",        SetType::RUN_QUERY},
          {"run.contract",     SetType::RUN_CONTRACT}
  };
// clang-format on

  class SetHandler {
  public:
    SetHandler() = default;

    std::vector<nlohmann::json> parseSet(std::vector<std::pair<tinyxml2::XMLElement *, std::string>> &set_nodes,
                                         ConditionManager &condition_manager, DataManager &data_manager);

  private:

    std::optional<nlohmann::json>
    handle(SetType set_type, tinyxml2::XMLElement *set_node, DataManager &data_manager, std::string &error);

  };

}

#endif
