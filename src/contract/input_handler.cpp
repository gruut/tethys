#include <algorithm>
#include <botan-2/botan/data_src.h>
#include <botan-2/botan/x509cert.h>
#include <cctype>
#include <regex>
#include <unordered_map>

#include "include/handler/input_handler.hpp"

namespace tethys::tsce {
bool tsce::InputHandler::parseInput(nlohmann::json &input_json, tinyxml2::XMLElement *input_node, DataManager &data_manager) {

  if (input_json.empty())
    return false;

  int num_input_max = input_node->IntAttribute("max", 1);

  if (num_input_max > MAX_INPUT_SIZE)
    return false;

  int num_input = num_input_max > input_json.size() ? input_json.size() : num_input_max;

  auto option_nodes = XmlTool::parseChildrenFromNoIf(input_node, "option");

  std::vector<InputOption> input_options;
  std::map<std::string, std::vector<std::string>> input_value_groups;

  for (auto each_node : option_nodes) {
    std::string option_name = mt::c2s(each_node->Attribute("name"));
    std::string option_type = mt::c2s(each_node->Attribute("type"));
    std::string option_validation = mt::c2s(each_node->Attribute("validation"));

    input_options.emplace_back(option_name, option_type, option_validation);
  }

  for (int i = 0; i < num_input; ++i) {

    for (auto &each_input : input_json[i]) {
      for (auto &each_item : each_input.items()) {
        std::string input_key = each_item.key();
        std::string input_value = each_item.value();

        bool is_valid_key = false;

        int opt_idx = -1;
        for (int j = 0; j < input_options.size(); ++j) {
          if (input_options[j].key == input_key) {
            is_valid_key = true;
            opt_idx = j;
            break;
          }
        }

        if (!is_valid_key || !isValidValue(input_value, input_options[opt_idx].type, input_options[opt_idx].validation))
          continue;

        std::string data_key = "$tx.contract.input[" + to_string(i) + "]." + input_key;
        std::string short_key = "$" + to_string(i) + "." + input_key;

        data_manager.updateValue(data_key, input_value);
        data_manager.updateValue(short_key, input_value);

        input_value_groups[input_key].emplace_back(input_value);

        if (input_options[opt_idx].type == "PEM") {
          Botan::DataSource_Memory cert_datasource(input_value);
          Botan::X509_Certificate cert(cert_datasource);
          auto not_after_x509 = cert.not_after();
          auto not_before_x509 = cert.not_before();
          auto not_after_epoch = not_after_x509.time_since_epoch();
          auto not_before_epoch = not_before_x509.time_since_epoch();
          std::string not_after_str = to_string(not_after_epoch);
          std::string not_before_str = to_string(not_before_epoch);

          auto data_key_prefix = "$tx.contract.input[" + to_string(i) + "].";
          auto short_key_prefix = "$" + to_string(i) + ".";
          data_manager.updateValue(data_key_prefix + "notafter", not_after_str);
          data_manager.updateValue(short_key_prefix + "notafter", not_after_str);
          data_manager.updateValue(data_key_prefix + "notbefore", not_before_str);
          data_manager.updateValue(short_key_prefix + "notbefore", not_before_str);

          auto sn = cert.serial_number();
          std::string sn_str(sn.begin(), sn.end());
          data_manager.updateValue(data_key_prefix + "sn", sn_str);
          data_manager.updateValue(short_key_prefix + "sn", sn_str);
        }
      }
    }
  }

  for (auto &it_map : input_value_groups) {
    std::string data_key = "$input@" + it_map.first;
    std::string data_value = nlohmann::json(it_map.second).dump();
    data_manager.updateValue(data_key, data_value);
  }

  return true;
}
}