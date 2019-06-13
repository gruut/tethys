#include "include/config.hpp"
#include "include/datamap.hpp"
#include "include/handler/get_handler.hpp"
#include "include/xml_tool.hpp"

namespace tethys::tsce {

  void tsce::GetHandler::parseGet(std::vector<std::pair<tinyxml2::XMLElement *, std::string>> &get_nodes,
                                  ConditionManager &condition_manager, DataManager &data_storage) {
    if(get_nodes.empty())
      return;

    for(auto &[get_node, condition_id] : get_nodes) {

      if(!condition_id.empty() && !condition_manager.getEvalResultById(condition_id))
        continue;

      auto var_nodes = XmlTool::parseChildrenFromNoIf(get_node,"var");

      for(auto var_node : var_nodes){
        std::string scope = mt::c2s(var_node->Attribute("scope"));

        if(!mt::inArray(scope,{"author","user","contract"}))
          continue;

        std::string id = mt::c2s(var_node->Attribute("id"));
        std::string id_as = mt::c2s(var_node->Attribute("id-as"));
        std::string name = mt::c2s(var_node->Attribute("name"));
        std::string name_as = mt::c2s(var_node->Attribute("as"));

        id = data_storage.eval(id);
        name = data_storage.eval(name);

        if(name.empty())
          continue;

        std::string storage_scope = (scope == "author" || scope == "user") ? "user" : "contract";
        std::string storage_id = id;

        if(scope == "author") {
          storage_id = data_storage.eval("$author");
        } else if(scope == "user") {
          if(id.empty())
            storage_id = data_storage.eval("$user");
        } else { // scope == "contract"
          if(id.empty())
            storage_id = data_storage.eval("$tx.body.cid");
        }

        auto scope_values = data_storage.getScopeVariables(storage_scope, storage_id, name);

        if(scope_values.empty())
          continue;

        if(id.empty() || id == storage_id) {
          for (auto &each_attr : scope_values) {
            std::string key = "$";
            key.append(scope).append(".").append(each_attr.name);
            data_storage.updateValue(key, each_attr.value);
          }
        }

        if(!id.empty() && id != storage_id && !id_as.empty()) {
          for (auto &each_attr : scope_values) {
            std::string key = "$.";
            key.append(id_as).append(".").append(each_attr.name);
            data_storage.updateValue(key, each_attr.value);
          }
        }

        if(!name_as.empty()) {

          if(scope_values.size() == 1) {
            data_storage.updateValue("$." + name_as, scope_values[0].value);
          } else {
            for (auto &each_attr : scope_values) {
              std::string key = "$.";
              key.append(name_as).append(".").append(each_attr.name);
              data_storage.updateValue(key, each_attr.value);
            }
          }
        }
      }

    }

  }
}