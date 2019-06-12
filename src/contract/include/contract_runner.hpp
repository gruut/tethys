#ifndef TETHYS_SCE_CONTRACT_RUNNER_HPP
#define TETHYS_SCE_CONTRACT_RUNNER_HPP

#include "../../../include/json.hpp"
#include "../../plugins/chain_plugin/structure/transaction.hpp"
#include "data_manager.hpp"
#include "condition_manager.hpp"
#include "handler/input_handler.hpp"
#include "handler/get_handler.hpp"
#include "handler/set_handler.hpp"
#include "element_parser.hpp"
#include "handler/fee_handler.hpp"

namespace tethys::tsce {

  class ContractRunner {

  public:
    ContractRunner() = default;

    void attachReadInterface(std::function<nlohmann::json(nlohmann::json & )> &read_storage_interface);

    void clear();

    bool loadWorldChain();

    bool setWorldChain();

    bool setContract(std::string &xml_doc);

    bool setTransaction(Transaction &proc_tx, std::string &error);

    bool readUserAttributes();

    nlohmann::json run();

  private:

    template<typename S = std::string>
    void attrToMap(std::vector <DataAttribute> &attr_list, S &&prefix) {
      if (attr_list.empty())
        return;

      for (auto &each_attr : attr_list) {
        std::string key = prefix;
        key.append(".").append(each_attr.name);
        m_data_manager.updateValue(key, each_attr.value);
      }

    }

    DataManager m_data_manager;
    ConditionManager m_condition_manager;
    InputHandler m_input_handler;
    GetHandler m_get_handler;
    SetHandler m_set_handler;
    ElementParser m_element_parser;
    Transaction m_proc_tx;
    FeeHandler m_fee_handler;

    std::vector <DataAttribute> m_world_attr;
    std::vector <DataAttribute> m_chain_attr;
  };
}

#endif
