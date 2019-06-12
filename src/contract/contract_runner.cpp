#include "include/config.hpp"
#include "include/contract_runner.hpp"
#include "include/handler/time_handler.hpp"

namespace tethys::tsce {

  void ContractRunner::attachReadInterface(std::function<nlohmann::json(nlohmann::json & )> &read_storage_interface) {
    m_data_manager.attachReadInterface(read_storage_interface);
  }

  void ContractRunner::clear() {
    m_data_manager.clear();
  }

  bool ContractRunner::loadWorldChain() {
    auto world_attr = m_data_manager.getWorld();
    auto chain_attr = m_data_manager.getChain();

    if (world_attr.empty() || chain_attr.empty()) {
      return false;
    }

    m_world_attr = world_attr;
    m_chain_attr = chain_attr;

    return true;
  }

  bool ContractRunner::setWorldChain() {

    if (m_world_attr.empty() || m_chain_attr.empty())
      return false;

    attrToMap(m_world_attr, "$world");
    attrToMap(m_chain_attr, "$chain");

    m_data_manager.setKeyCurrencyName(m_data_manager.eval("$world.keyc_name"));

    return true;
  }

  bool ContractRunner::setContract(std::string &xml_doc) {
    return m_element_parser.setContract(xml_doc);
  }

  bool ContractRunner::setTransaction(const Transaction &proc_tx, std::string &error) {

    m_proc_tx = proc_tx;

    auto time = proc_tx.getTxTime();
    auto cid = proc_tx.getContractId();
    auto receiver = proc_tx.getReceiverId();
    auto user_fee = proc_tx.getFee();
    auto user_id = proc_tx.getUserId();
    auto txid = proc_tx.getTxId();

    if (cid.empty() || receiver.empty() || user_id.empty() || txid.empty()) {
      error = "missing elements";
      return false;
    }

    if (user_fee < MIN_USER_FEE) {
      error = "less than minimum user fee";
      return false;
    }

    auto user_pk = proc_tx.getTxUserPk();

    m_data_manager.updateValue("$tx.txid", txid);
    m_data_manager.updateValue("$txid", txid);

    m_data_manager.updateValue("$tx.time", std::to_string(time));
    m_data_manager.updateValue("$time", std::to_string(time));

    std::vector<std::string> cid_components = mt::split(cid, "::");

    m_data_manager.updateValue("$tx.body.cid", cid);
    m_data_manager.updateValue("$cid", cid);
    m_data_manager.updateValue("$author", cid_components[1]);
    m_data_manager.updateValue("$chain", cid_components[2]);
    m_data_manager.updateValue("$world", cid_components[3]);
    m_data_manager.updateValue("$tx.body.receiver", receiver);
    m_data_manager.updateValue("$receiver", receiver);

    m_data_manager.updateValue("$tx.body.fee", std::to_string(user_fee));
    m_data_manager.updateValue("$fee", std::to_string(user_fee));

    m_data_manager.updateValue("$tx.user.id", user_id);
    m_data_manager.updateValue("$user", user_id);

    m_data_manager.updateValue("$tx.user.pk", user_pk);

    auto tx_endorsers = proc_tx.getEndorsers();

    for (int i = 0; i < tx_endorsers.size(); ++i) {
      std::string id_key = "$tx.endorser[" + to_string(i) + "].id";
      std::string pk_key = "$tx.endorser[" + to_string(i) + "].pk";
      m_data_manager.updateValue(id_key, tx_endorsers[i].endorser_id);
      m_data_manager.updateValue(pk_key, tx_endorsers[i].endorser_pk);
    }

    m_data_manager.updateValue("$tx.endorser.count", std::to_string(tx_endorsers.size()));

    return true;
  }

  bool ContractRunner::readUserAttributes() {

    auto user_attr = m_data_manager.getUserInfo("$user");
    auto receiver_attr = m_data_manager.getUserInfo("$receiver");
    auto author_attr = m_data_manager.getUserInfo("$author");

    if (user_attr.empty() || receiver_attr.empty() || author_attr.empty())
      return false;

    attrToMap(user_attr, "$user");
    attrToMap(receiver_attr, "$receiver");
    attrToMap(author_attr, "$author");

    return true;


#if 0
    int num_endorser;
      try{
        num_endorser = std::stoi(m_data_manager.eval("$tx.endorser.count"));
      }
      catch(...){
        num_endorser = 0;
      }

      for(int i = 0; i < num_endorser; ++i) {
        std::string prefix = "$tx.endorser[" + to_string(i) + "]";
        auto endorser_attr = m_data_manager.getUserInfo(prefix + ".id");
        attrToMap(endorser_attr, prefix);
      }
#endif
  }

  nlohmann::json ContractRunner::run() {

    nlohmann::json result_query = R"(
      {
        "txid":"",
        "status":true,
        "info":"",
        "authority": {
          "author": "",
          "user": "",
          "receiver": "",
          "self": "",
          "friend":[]
        },
        "fee": {
          "author": "",
          "user": ""
        },
        "queries": []
      })"_json;

    auto &data_map = m_data_manager.getDatamap();

    result_query["txid"] = m_data_manager.eval("$tx.txid");

    auto head_node = m_element_parser.getNode("head");
    auto &condition_nodes = m_element_parser.getNodes("condition");

    // check if contract is runnable

    TimeHandler contract_time_handler;
    if (!contract_time_handler.evalue(head_node, m_data_manager)) {
      result_query["status"] = false;
      result_query["info"] = TSCE_ERROR_MSG.at("RUN_PERIOD");
      return result_query;
    }

    // OK ready to go

    for (auto &each_condition : condition_nodes) {
      m_condition_manager.evalue(each_condition.first, m_data_manager);
    }
    result_query["authority"]["author"] = m_data_manager.eval("$author");
    result_query["authority"]["user"] = m_data_manager.eval("$user");
    result_query["authority"]["receiver"] = m_data_manager.eval("$receiver");
    result_query["authority"]["self"] = m_data_manager.eval("$tx.body.cid");

    // TODO : result_query["authority"]["friend"]

    // process input directive

    auto input_node = m_element_parser.getNode("input");

    auto tx_input_json = nlohmann::json::from_cbor(m_proc_tx.getTxInputCbor());

    if (!m_input_handler.parseInput(tx_input_json, input_node, m_data_manager)) {
      result_query["status"] = false;
      result_query["info"] = TSCE_ERROR_MSG.at("RUN_INPUT");
      return result_query;
    }

    // process get directive

    for (auto &each_condition : condition_nodes) {
      m_condition_manager.evalue(each_condition.first, m_data_manager);
    }

    auto &get_nodes = m_element_parser.getNodes("get");
    m_get_handler.parseGet(get_nodes, m_condition_manager, m_data_manager);

    // TODO : oracle handler (pending)
    // TODO : script handler (pending)

    // no more change values here

    for (auto &each_condition : condition_nodes) {
      m_condition_manager.evalue(each_condition.first, m_data_manager);
    }

    // process fee directive

    auto &fee_nodes = m_element_parser.getNodes("fee");
    //auto [pay_from_user, pay_from_author] = m_fee_handler.parseGet(fee_nodes,m_condition_manager, m_data_manager);
    auto pay_from = m_fee_handler.parseGet(fee_nodes, m_condition_manager, m_data_manager);

    if (!pay_from) {
      result_query["status"] = false;
      result_query["info"] = TSCE_ERROR_MSG.at("RUN_FEE");
      return result_query;
    }

    auto &[pay_from_user, pay_from_author] = pay_from.value();

    if (pay_from_user > 0 && m_data_manager.getUserKeyCurrency("$user") < pay_from_user) {
      result_query["status"] = false;
      result_query["info"] = TSCE_ERROR_MSG.at("NOT_ENOUGH_FEE") + " (user)";
      return result_query;
    }

    if (pay_from_author > 0 && m_data_manager.getUserKeyCurrency("$author") < pay_from_author) {
      result_query["status"] = false;
      result_query["info"] = TSCE_ERROR_MSG.at("NOT_ENOUGH_FEE") + " (author)";
      return result_query;
    }

    result_query["fee"]["user"] = std::to_string(pay_from_user);
    result_query["fee"]["author"] = std::to_string(pay_from_author);

    // process set directive

    auto &set_nodes = m_element_parser.getNodes("set");
    auto set_queries = m_set_handler.parseSet(set_nodes, m_condition_manager, m_data_manager);

    if (set_queries.empty()) {
      result_query["status"] = false;
      result_query["info"] = TSCE_ERROR_MSG.at("RUN_SET") + " or " + TSCE_ERROR_MSG.at("RUN_TAG");
    } else {
      result_query["queries"] = set_queries;
    }

    return result_query;
  }
}
