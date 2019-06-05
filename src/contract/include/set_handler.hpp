#ifndef TETHYS_SCE_SET_HANDLER_HPP
#define TETHYS_SCE_SET_HANDLER_HPP

#include "datamap.hpp"
#include "data_manager.hpp"
#include "condition_manager.hpp"
#include "tag_handler.hpp"

namespace tethys::tsce {

const auto REGEX_BASE64 = "^(?:[A-Za-z0-9+/]{4})*(?:[A-Za-z0-9+/]{3}=|[A-Za-z0-9+/]{2}==)?$";

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
const std::map<std::string, SetType> SET_TYPE_MAP {
  {"user.cert", SetType::USER_JOIN},
  {"user.cert", SetType::USER_CERT},
  {"v.create", SetType::V_CREATE},
  {"v.incinerate", SetType::V_INCINERATE},
  {"v.transfer", SetType::V_TRANSFER},
  {"scope.user", SetType::SCOPE_USER},
  {"scope.contract", SetType::SCOPE_CONTRACT},
  {"contract.new", SetType::CONTRACT_NEW},
  {"contract.disable", SetType::CONTRACT_DISABLE},
  {"trade.item", SetType::TRADE_ITEM},
  {"trade.v", SetType::TRADE_V},
  {"run.query", SetType::RUN_QUERY},
  {"run.contract", SetType::RUN_CONTRACT}
};
// clang-format on

class SetHandler {
public:
  SetHandler() = default;

  std::vector<nlohmann::json> parseSet(std::vector<std::pair<tinyxml2::XMLElement*,std::string>> &set_nodes, ConditionManager &condition_manager, DataManager &data_manager){
    std::vector<nlohmann::json> set_queries;

    for(auto &[set_node, if_id] : set_nodes){
      if(set_node == nullptr)
        continue;

      if(!if_id.empty() && !condition_manager.getEvalResultById(if_id))
        continue;

      std::string type_str = mt::c2s(set_node->Attribute("type"));

      auto it = SET_TYPE_MAP.find(type_str);
      auto set_type = (it == SET_TYPE_MAP.end() ? SetType::NONE : it->second);

      if(set_type != SetType::NONE) {

        nlohmann::json set_query;
        set_query["type"] = type_str;

        std::string error;

        auto contents = handle(set_type, set_node, data_manager, error);

        if (!contents)
          continue;

        set_query["option"] = contents.value();
        set_queries.emplace_back(set_query);

      }
    }

    return set_queries;
  }

private:

  std::optional<nlohmann::json> handle(SetType set_type, tinyxml2::XMLElement* set_node, DataManager &data_manager, std::string &error) {

    // 1) parsing node attributes

    nlohmann::json contents;

    std::string from_att;
    std::string for_att;

    if(set_type == SetType::SCOPE_USER) {
      for_att = mt::c2s(set_node->Attribute("for"));
      if (for_att.empty() || !mt::inArray(for_att,{"user","author"})) {
        error = TSCE_ERROR_MSG["RUN_SET"];
        return std::nullopt;
      }

      contents["for"] = for_att;
    }
    else if(set_type == SetType::V_TRANSFER) {
      from_att = mt::c2s(set_node->Attribute("from"));
      if (from_att.empty() || !mt::inArray(from_att,{"user","author","contract"})) {
        error = TSCE_ERROR_MSG["RUN_SET"];
        return std::nullopt;
      }

      contents["from"] = from_att;
    }

    // 2) parsing all option entries

    auto option_nodes = XmlTool::parseChildrenFromNoIf(set_node,"option");

    for (auto &option_node : option_nodes) {

      std::string option_name = mt::c2s(option_node->Attribute("name"));
      std::string option_value = mt::c2s(option_node->Attribute("value"));
      std::string data = data_manager.eval(option_value);

      mt::trim(data);
      mt::toLower(option_name);

      if (option_name.empty() || data.empty())
        continue;

      switch (set_type) {

      case SetType::USER_JOIN: {
        if (option_name == "gender") {
          data = mt::toUpper(data);
          if (!mt::inArray(data,{"MALE","FEMALE","OTHER"}))
            data.clear(); // for company or something other cases
        }

        if (option_name == "register_day") {
          // TODO : fix to YYYY-MM-DD format
          if (!mt::isDigit(data))
            data.clear();
        }

        break;
      }

      case SetType::USER_CERT: {
        if (mt::inArray(option_name,{"notbefore","notafter"})) {
          // TODO : fix to allow both YYYY-MM-DD and timestamp
          if (!mt::isDigit(data))
            data.clear();
        }

        if (option_name == "x509") {
          string begin_str = "-----BEGIN CERTIFICATE-----";
          string end_str = "-----END CERTIFICATE-----";
          auto found1 = data.find(begin_str);
          auto found2 = data.find(end_str);
          if (found1 == string::npos || found2 == string::npos)
            data.clear();

          auto content_len = data.length() - begin_str.length() - end_str.length();
          auto content = data.substr(begin_str.length(), content_len);
          std::regex rgx(REGEX_BASE64);
          if (!std::regex_match(data, rgx))
            data.clear();
        }

        break;
      }

      case SetType::V_CREATE: {
        if (option_name == "amount") {
          if (mt::str2num<int>(data) <= 0)
            data.clear();
        }

        if (option_name == "type") {
          mt::toUpper(data);
          if (!mt::inArray(data,{"KEYC","FIAT","COIN","XCOIN","MILE"}))
            data.clear();
        }

        break;
      }

      case SetType::V_INCINERATE: {
        if (option_name == "amount") {
          // TODO : write code for checking amount
        }

        if (option_name == "pid") {
          std::regex rgx(REGEX_BASE64);
          if (!std::regex_match(data, rgx))
            data.clear();
        }
        break;
      }

      case SetType::SCOPE_USER: {
        if(option_name == "tag") {
          // TODO : check data is xml
        }
        break;
      }

      case SetType::SCOPE_CONTRACT: {
        if(option_name == "pid") {
          std::regex rgx(REGEX_BASE64);
          if (!std::regex_match(data, rgx))
            data.clear();
        }
        break;
      }


      case SetType::CONTRACT_NEW: {
        if (mt::inArray(option_name, {"before","after"})) {
          // TODO : fix to allow both YYYY-MM-DD and timestamp
          if (!mt::isDigit(data))
            data.clear();
        }
        break;
      }

      case SetType::CONTRACT_DISABLE: {
        // TODO : check cid is belong to user
        break;
      }

      case SetType::V_TRANSFER: {
        if(!data.empty()) {
          if (option_name == "tag") {
            // TODO : check data must be xml
          }

          if (option_name == "amount") {

            mt::trim(data);
            if (mt::str2num<int>(data) <= 0)
              data.clear();

          }
        }

        break;
      }

      case SetType::RUN_QUERY: {
        if (option_name == "type") {
          if (!mt::inArray(data, {"run.query","user.cert"}))
            data.clear();
        }

        if (option_name == "after") {
          // TODO : fix to allow both YYYY-MM-DD and timestamp
          if (!mt::isDigit(data))
            data.clear();
        }

        break;
      }


      case SetType::RUN_CONTRACT: {
          //TODO : check 'cid'
        if (option_name == "after") {
          // TODO : fix to allow both YYYY-MM-DD and timestamp
          if (!mt::isDigit(data))
            data.clear();
        }

        break;
      }
      default:
      break;
      }

      if(!data.empty())
        contents[option_name] = data;
    }

    // checking tag condition

    if(set_type == SetType::SCOPE_USER || (set_type == SetType::V_TRANSFER  && (from_att == "user" || from_att == "author"))) {
      std::string id = data_manager.eval("$" + from_att);
      std::string pid = JsonTool::get<std::string>(contents, "pid").value_or("");
      std::string name = (set_type == SetType::SCOPE_USER) ? JsonTool::get<std::string>(contents, "name").value_or("") : JsonTool::get<std::string>(contents,"unit").value_or("");

      if (!pid.empty()) {
        auto record = data_manager.getUserScopeRecordByPid(id, name, pid);

        if (!record) {
          error = TSCE_ERROR_MSG["NO_RECORD"];
          return std::nullopt;
        }

        if (!record.value().tag.empty()) {

          TagHandler tag_handler;
          if(!tag_handler.evalue(record.value().tag,data_manager)) {
            error = TSCE_ERROR_MSG["RUN_TAG"];
            return std::nullopt;
          }

        }
      }
    }

    return contents;
  }

};

}

#endif
