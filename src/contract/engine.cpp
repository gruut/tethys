#include "include/engine.hpp"
#include "include/config.hpp"
#include "include/contract_runner.hpp"
#include "include/tx_parallelizer.hpp"

namespace tethys::tsce {

void ContractEngine::attachReadInterface(std::function<nlohmann::json(nlohmann::json &)> read_storage_interface) {
  m_storage_interface = read_storage_interface;
  m_contract_manager.attachReadInterface(read_storage_interface);
}

std::optional<nlohmann::json> ContractEngine::procBlock(Block &block) {
  logger::INFO("called procBlock");
  if (m_storage_interface == nullptr)
    return std::nullopt;

  if (block.getNumTransaction() <= 0) {
    return std::nullopt;
  }

  block_height_type block_hgt = block.getHeight();
  base58_type block_id = block.getBlockId();

  std::vector<nlohmann::json> result_queries;

  ContractRunner contract_runner;
  contract_runner.attachReadInterface(m_storage_interface);

  if (!contract_runner.loadWorldChain())
    return std::nullopt;

  for (auto each_tx : block.getTransactions()) {

    auto txid = each_tx.getTxId();
    auto cid = each_tx.getContractId();

    std::string error;

    nlohmann::json result_fail;
    result_fail["status"] = false;
    result_fail["txid"] = txid;

    if (txid.empty() || cid.empty()) {
      result_fail["info"] = TSCE_ERROR_MSG.at("INVALID_TX");
      result_queries.emplace_back(result_fail);
      continue;
    }

    contract_runner.clear();

    if (!contract_runner.setWorldChain()) {
      result_fail["info"] = TSCE_ERROR_MSG.at("CONFIG_WORLD");
      result_queries.emplace_back(result_fail);
      continue;
    }

    auto contract = m_contract_manager.getContract(cid);

    if (!contract.has_value()) {
      result_fail["info"] = TSCE_ERROR_MSG.at("NO_CONTRACT");
      result_queries.emplace_back(result_fail);
      continue;
    }

    if (!contract_runner.setContract(contract.value())) {
      result_fail["info"] = TSCE_ERROR_MSG.at("NO_CONTRACT");
      result_queries.emplace_back(result_fail);
      continue;
    }

    if (!contract_runner.setTransaction(each_tx, error)) {
      result_fail["info"] = TSCE_ERROR_MSG.at("INVALID_TX") + " (" + error + ")";
      result_queries.emplace_back(result_fail);
      continue;
    }

    if (!contract_runner.readUserAttributes()) {
      result_fail["info"] = TSCE_ERROR_MSG.at("NO_USER");
      result_queries.emplace_back(result_fail);
      continue;
    }

    auto res_query = contract_runner.run();
    result_queries.emplace_back(res_query);
  }
  logger::INFO("return procBlock");
  return m_query_composer.compose(result_queries, block_id, block_hgt);
}
}