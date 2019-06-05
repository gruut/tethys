#ifndef TETHYS_SCE_CONDITION_HANDLER_HPP
#define TETHYS_SCE_CONDITION_HANDLER_HPP

#include "config.hpp"
#include "base_condition_handler.hpp"
#include "handler_compare.hpp"
#include "handler_signature.hpp"
#include "handler_certificate.hpp"
#include "handler_time.hpp"
#include "handler_endorser.hpp"
#include "handler_user.hpp"
#include "data_manager.hpp"
#include "handler_var.hpp"

#include <algorithm>
#include <iostream>
#include <map>

namespace tethys::tsce {

class ConditionHandler : BaseConditionHandler {

public:
  ConditionHandler() = default;

  bool evalue(tinyxml2::XMLElement* doc_node, DataManager &data_manager) override {

    if(doc_node == nullptr)
      return false;

    PrimaryConditionType base_condition_type = getPrimaryConditionType(mt::c2s(doc_node->Name()));
    EvalRuleType base_eval_rule = getEvalRule(mt::c2s(doc_node->Attribute("eval-rule"))).value_or(EvalRuleType::OR);

    bool eval_result = true;

    switch(base_condition_type) {
    case PrimaryConditionType::ROOT:
    case PrimaryConditionType::IF:
    case PrimaryConditionType::NIF: {

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
    case PrimaryConditionType::COMPARE: {
      CompareHandler compare_handler;
      eval_result = compare_handler.evalue(doc_node, data_manager);
      break;
    }
    case PrimaryConditionType::SIGNATURE: {
      SignatureHandler signature_handler;
      eval_result = signature_handler.evalue(doc_node, data_manager);
      break;
    }
    case PrimaryConditionType::CERTIFICATE: {
      CertificateHandler certificate_handler;
      eval_result = certificate_handler.evalue(doc_node, data_manager);
      break;
    }
    case PrimaryConditionType::TIME: {
      TimeHandler time_handler;
      eval_result = time_handler.evalue(doc_node, data_manager);
      break;
    }
    case PrimaryConditionType::ENDORSER: {
      EndorserHandler endorser_handler;
      eval_result = endorser_handler.evalue(doc_node, data_manager);
      break;
    }
    case PrimaryConditionType::RECEIVER:
    case PrimaryConditionType::USER: {
      UserHandler user_handler;
      user_handler.setUserType(base_condition_type);
      eval_result = user_handler.evalue(doc_node, data_manager);
      break;
    }
    case PrimaryConditionType::VAR: {
      VarHandler var_handler;
      eval_result = var_handler.evalue(doc_node, data_manager);
      break;
    }
    default:
      break;
    }

    if(base_condition_type == PrimaryConditionType::NIF) {
      eval_result = !eval_result;
    }

    return eval_result;
  }

};

}

#endif
