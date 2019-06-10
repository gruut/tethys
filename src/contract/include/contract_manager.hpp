#ifndef TETHYS_SCE_CONTRACT_MANAGER_HPP
#define TETHYS_SCE_CONTRACT_MANAGER_HPP

#include "../../../include/json.hpp"

namespace tethys::tsce {

  using namespace nlohmann;

  class ContractManager {
  public:
    ContractManager() = default;

    void attachReadInterface(std::function<nlohmann::json(nlohmann::json & )> contract_storage_interface);

    template<typename S = std::string>
    std::optional<std::string> getContract(S &&cid) {

      if (m_contract_storage_interface == nullptr) {
        return std::nullopt;
      }

      // TODO : validating of cid format

      auto it_map = m_contract_cache.find(cid);
      if (it_map != m_contract_cache.end()) {

        if (it_map->second.empty()) {
          return std::nullopt;
        } else {
          return it_map->second;
        }
      }

      nlohmann::json query = {
              {"type",  "contract.get"},
              {"where", {
                                {"cid", cid}
                        }}
      };

      nlohmann::json query_result = m_contract_storage_interface(query);

      if (query_result.empty() || query_result["data"].empty() || query_result["data"][0].empty()) {
        m_contract_cache.insert({cid, ""});
        return std::nullopt;
      }

      auto xml_doc = query_result["data"][0][0].get<std::string>();

      m_contract_cache.insert({cid, xml_doc});

      if (xml_doc.empty())
        return std::nullopt;

      return xml_doc;
    }

  private:
    std::map<std::string, std::string> m_contract_cache;
    std::function<nlohmann::json(nlohmann::json & )> m_contract_storage_interface;

  };

}

#endif
