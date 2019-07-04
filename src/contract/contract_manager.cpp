#include "include/contract_manager.hpp"
#include "include/config.hpp"

namespace tethys::tsce {

void ContractManager::attachReadInterface(std::function<nlohmann::json(nlohmann::json &)> contract_storage_interface) {
  m_contract_storage_interface = contract_storage_interface;
}

}