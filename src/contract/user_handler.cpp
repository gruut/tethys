#include "include/config.hpp"
#include "include/handler/user_handler.hpp"

namespace tethys::tsce {

  UserHandler::UserHandler() {
    m_user_key = "$user";
  }

  void UserHandler::setUserType(PrimaryConditionType user_type) {
    if(user_type == PrimaryConditionType::RECEIVER) {
      m_user_key = "$receiver";
    }
  }

  bool UserHandler::evalue(tinyxml2::XMLElement *doc_node, DataManager &data_manager) {

    if(doc_node == nullptr)
      return false;

    SecondaryConditionType base_condition_type = getSecondaryConditionType(mt::c2s(doc_node->Name()));

    bool eval_result;

    switch(base_condition_type) {
      case SecondaryConditionType::USER:
      case SecondaryConditionType::RECEIVER:
      case SecondaryConditionType::IF:
      case SecondaryConditionType::NIF: {
        EvalRuleType base_eval_rule = getEvalRule(mt::c2s(doc_node->Attribute("eval-rule"))).value_or(EvalRuleType::AND);

        if (base_eval_rule == EvalRuleType::AND) {
          eval_result = true;
          tinyxml2::XMLElement *element_ptr;
          element_ptr = doc_node->FirstChildElement();
          while(element_ptr) {
            eval_result &= evalue(element_ptr, data_manager);
            element_ptr = element_ptr->NextSiblingElement();
          }
        } else {
          eval_result = false;
          tinyxml2::XMLElement *element_ptr;
          element_ptr = doc_node->FirstChildElement();
          while(element_ptr) {
            eval_result |= evalue(element_ptr, data_manager);
            element_ptr = element_ptr->NextSiblingElement();
          }
        }
        break;
      }
      case SecondaryConditionType::ID: {
        std::string contract_user_id_b58 = mt::c2s(doc_node->GetText()); // <id>...</id>
        mt::trim(contract_user_id_b58);

        std::string user_id = data_manager.eval(m_user_key);

        eval_result = (user_id == contract_user_id_b58);
        break;
      }
      case SecondaryConditionType::LOCATION: {

        // TODO : improve smarter

        std::string location = mt::c2s(doc_node->Attribute("country"));
        location.append(" ").append(mt::c2s(doc_node->Attribute("state")));

        mt::trim(location);

        auto data = data_manager.evalOpt(m_user_key + ".location");
        if(!data.has_value())
          return false;

        eval_result = (data.value() == location);
        break;
      }

      case SecondaryConditionType::SERVICE: {

        // TODO : improve smarter

        std::string service_type = mt::c2s(doc_node->Attribute("type"));
        std::string service_code = mt::c2s(doc_node->GetText());

        mt::trim(service_code);

        auto type_data = data_manager.evalOpt(m_user_key + ".isc_type");
        if(!type_data.has_value())
          return false;

        auto code_data = data_manager.evalOpt(m_user_key + ".isc_code");
        if(!code_data.has_value())
          return false;

        eval_result = (type_data.value() == service_type && code_data.value() == service_code);
        break;

      }

      case SecondaryConditionType::GENDER: {

        std::string gender_str = mt::c2s(doc_node->GetText());
        mt::trim(gender_str);
        mt::toUpper(gender_str);

        auto gender_data = data_manager.evalOpt(m_user_key + ".gender");

        if(!gender_data.has_value())
          return false;

        eval_result = (gender_str == gender_data.value());

        break;
      }

      case SecondaryConditionType::AGE: {

        std::string age_after = mt::c2s(doc_node->Attribute("after"));
        std::string age_before = mt::c2s(doc_node->Attribute("before"));

        mt::trim(age_after);
        mt::trim(age_before);

        if(age_after.empty() && age_before.empty())
          return false;

        std::string birthday_str = data_manager.eval(m_user_key + ".birthday");

        if(birthday_str.empty())
          return false;

        std::istringstream birthday_in{birthday_str};
        date::sys_time<std::chrono::milliseconds> birthday_time_point;
        birthday_in >> date::parse("%F", birthday_time_point);

        if(birthday_in.fail())
          return false;

        std::string now_str = data_manager.eval("$time"); // timestamp in second

        auto now_int = mt::str2num<uint64_t>(now_str) * 1000;

        if(now_int == 0)
          return false;

        std::chrono::milliseconds now_chrono_ms(now_int);
        date::sys_time<std::chrono::milliseconds> now_time_point(now_chrono_ms);

        auto birthday = date::year_month_day{floor<date::days>(birthday_time_point)};
        auto today = date::year_month_day{floor<date::days>(now_time_point)};

        if(age_before.empty()) {
          int age_after_int = mt::str2num<int>(age_after);
          return (birthday + date::years{age_after_int} < today);
        }

        if(age_after.empty()) {
          int age_before_int = mt::str2num<int>(age_before);
          return (birthday + date::years{age_before_int} > today);
        }

        int age_after_int = mt::str2num<int>(age_after);
        int age_before_int = mt::str2num<int>(age_before);
        return (birthday + date::years{age_after_int} < today && today < birthday + date::years{age_before_int});

      }
      default: {
        eval_result = false;
        break;
      }
    }

    if(base_condition_type == SecondaryConditionType::NIF) {
      eval_result = !eval_result;
    }

    return eval_result;
  }
}