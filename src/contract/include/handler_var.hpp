#ifndef TETHYS_SCE_HANDLER_VAR_HPP
#define TETHYS_SCE_HANDLER_VAR_HPP

#include "base_condition_handler.hpp"
#include "handler_compare.hpp"
namespace tethys::tsce {

class VarHandler : public BaseConditionHandler {
public:
  VarHandler() = default;

  bool evalue(tinyxml2::XMLElement* doc_node, DataManager &data_manager) override {

    if(doc_node == nullptr)
      return false;

    std::string scope_str = mt::c2s(doc_node->Attribute("scope"));
    std::string id_str = mt::c2s(doc_node->Attribute("id"));
    std::string name_str = mt::c2s(doc_node->Attribute("name"));
    std::string ref_str = mt::c2s(doc_node->Attribute("ref"));
    std::string type_str = mt::c2s(doc_node->Attribute("type"));
    std::string abs_str = mt::c2s(doc_node->Attribute("abs"));

    if(mt::trim(name_str) == "*")
      return false;

    tinyxml2::XMLDocument compare_doc; // to keep data

    tinyxml2::XMLElement* compare_node = compare_doc.NewElement("compare");
    compare_node->SetAttribute("ref", ref_str.c_str());
    compare_node->SetAttribute("type", type_str.c_str());
    compare_node->SetAttribute("abs", abs_str.c_str());

    CompareHandler compare_handler;

    if(mt::inArray(scope_str,{"user","author", "receiver","contract"})) {

      id_str = data_manager.eval(id_str);
      std::string real_scope = (scope_str == "contract") ? "contract" : "user";
      auto return_val = data_manager.getScopeVariables(real_scope, id_str, name_str); // should be [{name,value}]
      if (return_val.empty())
        return false;

      compare_node->SetAttribute("src", return_val[0].value.c_str());

      return compare_handler.evalue(compare_node,data_manager);

    } else if(scope_str == "transaction") {

      return false;

    } else if(scope_str == "block") {

      return false;

    } else if(scope_str == "world") {

      std::string keyw = "$world." + name_str;
      std::string src_val = data_manager.eval(keyw);
      compare_node->SetAttribute("src", src_val.c_str());
      return compare_handler.evalue(compare_node,data_manager);

    } else if(scope_str == "chain") {

      std::string keyw = "$chain." + name_str;
      std::string src_val = data_manager.eval(keyw);
      compare_node->SetAttribute("src", src_val.c_str());
      return compare_handler.evalue(compare_node,data_manager);
    } else {
      return false;
    }
  }
};

}

#endif
