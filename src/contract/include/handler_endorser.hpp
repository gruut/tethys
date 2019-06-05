#ifndef TETHYS_SCE_HANDLER_ENDORSER_HPP
#define TETHYS_SCE_HANDLER_ENDORSER_HPP

#include "config.hpp"
#include "base_condition_handler.hpp"

namespace tethys::tsce {

class EndorserHandler : public BaseConditionHandler {
public:
  EndorserHandler() = default;

  bool evalue(tinyxml2::XMLElement* doc_node, DataManager &data_manager) override {

    if(doc_node == nullptr)
      return false;


    SecondaryConditionType base_condition_type = getSecondaryConditionType(mt::c2s(doc_node->Name()));

    bool eval_result = false;

    switch(base_condition_type) {
    case SecondaryConditionType::ENDORSER:
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

      std::string endorser_id_b58 = mt::c2s(doc_node->GetText());

      mt::trim(endorser_id_b58);

      auto data = data_manager.evalOpt("$tx.endorser.count");
      if(!data.has_value()){
        return false;
      }

      int num_endorsers = mt::str2num<int>(data.value());

      if(num_endorsers <= 0) {
        return false;
      }

      for(int i = 0 ; i < num_endorsers; ++i) {
        std::string nth_endorder_id = "$tx.endorser[" + std::to_string(i) + "].id";
        auto tx_endorser = data_manager.evalOpt(nth_endorder_id).value_or("");

        if(!tx_endorser.empty() && tx_endorser == endorser_id_b58) {
          eval_result = true;
          break;
        }
      }

      break;
    }
    default:
      eval_result = false;
      break;
    }

    if(base_condition_type == SecondaryConditionType::NIF) {
      eval_result = !eval_result;
    }

    return eval_result;
  }




};

}

#endif
