#ifndef TETHYS_SCE_DATA_STORAGE_HPP
#define TETHYS_SCE_DATA_STORAGE_HPP

#include <string>
#include <string_view>

#include "datamap.hpp"
#include "types.hpp"
#include "misc_tool.hpp"
#include "../../../lib/tethys-utils/src/type_converter.hpp"
#include "../../../lib/json/json.cpp"

namespace tethys::tsce {

  using namespace std;

  struct DataAttribute {
    std::string name;
    std::string value;

    DataAttribute() = default;

    DataAttribute(const std::string &name_, const std::string &value_) : name(name_), value(value_) {}
  };

  struct UserScopeRecord {
    std::string var_name;
    std::string var_value;
    EnumAll var_type;
    std::string var_owner;
    uint64_t up_time;
    uint64_t up_block;
    std::string tag;
    std::string pid;
  };

  struct ContractScopeRecord {
    std::string contract_id;
    std::string var_name;
    std::string var_value;
    EnumAll var_type;
    std::string var_info;
    uint64_t up_time;
    uint64_t up_block;
    std::string pid;
  };

  struct UserAttributeRecord {
    std::string uid;
    std::string register_day;
    std::string register_code;
    EnumGender gender;
    std::string isc_type;
    std::string isc_code;
    std::string location;
    int age_limit;
  };

  struct UserCertRecord {
    std::string uid;
    std::string sn;
    uint64_t nvbefore;
    uint64_t nvafter;
    std::string x509;
  };

  class DataManager {
  public:
    DataManager() = default;

    void attachReadInterface(std::function<nlohmann::json(nlohmann::json & )> &read_storage_interface);

    template<typename S1 = std::string, typename S2 = std::string>
    void updateValue(S1 &&key, S2 &&value) {
      m_tx_datamap.set(key, value);
    }

    void setKeyCurrencyName(std::string_view keyc_name);

    std::string eval(std::string_view expr_view);

    std::string eval(std::string &expr);

    std::optional<std::string> evalOpt(std::string_view expr_view);

    std::optional<std::string> evalOpt(std::string &expr);

    template<typename S = std::string>
    std::optional<std::string> get(S &&expr) {
      return m_tx_datamap.get(expr);
    }

    void clear();

    Datamap &getDatamap();

    std::vector<DataAttribute> getWorld();

    std::vector<DataAttribute> getChain();

    std::vector<DataAttribute> getUserInfo(std::string_view user_id_);

    int64_t getUserKeyCurrency(std::string_view user_id_);

    template<typename S = std::string>
    std::vector<DataAttribute> getUserCert(S &&user_id_) {
      if (user_id_.empty())
        return {};

      std::string user_id = user_id_;

      if (user_id[0] == '$') {
        user_id = eval(user_id);
      }

      nlohmann::json query;
      query["type"] = "user.cert.get";
      query["where"]["uid"] = user_id;

      return queryIfUserCertAndParseData(query, user_id);
    }

    template<typename S1 = std::string, typename S2 = std::string>
    std::optional<UserScopeRecord> getUserScopeRecordByName(S1 &&user_id, S2 &&var_name) {

      if (var_name == "*")
        return std::nullopt;

      std::string scope_key = user_id + var_name;

      auto ret_record = findUserScopeTableByName(user_id, var_name);

      if (ret_record)
        return ret_record;

      nlohmann::json query;
      query["type"] = "user.scope.get";
      query["where"]["uid"] = user_id;
      query["where"]["name"] = var_name;

      queryIfUserScopeAndParseData(query, user_id, "", false); // try to cache - ignore return value;

      return findUserScopeTableByName(user_id, var_name);
    }

    template<typename S1 = std::string, typename S2 = std::string, typename S3 = std::string>
    std::optional<UserScopeRecord> getUserScopeRecordByPid(S1 &&user_id, S2 &&var_name, S3 &&pid) {

      std::string scope_key = user_id + var_name;

      auto ret_record = findUserScopeTableByPid(user_id, var_name, pid);

      if (ret_record)
        return ret_record;

      nlohmann::json query;
      query["type"] = "user.scope.get";
      query["where"]["uid"] = user_id;
      query["where"]["pid"] = pid;

      queryIfUserScopeAndParseData(query, user_id, "", false); // try to cache - ignore return value;

      return findUserScopeTableByPid(user_id, var_name, pid);
    }

    template<typename S1 = std::string, typename S2 = std::string, typename S3 = std::string>
    std::vector<DataAttribute> getScopeVariables(std::string_view scope, S2 &&id, S3 &&name) {

      if (scope.empty() || id.empty() || name.empty() || !(scope == "user" || scope == "contract"))
        return {};

      std::vector<DataAttribute> ret_vec;

      if (name != "*") {
        std::string scope_key = id + name;
        if (scope == "user") {
          auto it_tbl = m_user_scope_table.find(scope_key);
          if (it_tbl != m_user_scope_table.end() && !it_tbl->second.empty()) {
            for (auto &each_row : it_tbl->second) {
              if (each_row.tag.empty()) {
                ret_vec.emplace_back(name, each_row.var_value);
                break;
              }
            }

          }
        } else { // contract
          auto it_tbl = m_contract_scope_table.find(scope_key);
          if (it_tbl != m_contract_scope_table.end()) {
            ret_vec.emplace_back(name, it_tbl->second.var_value);
          }
        }

      } else {
        if (scope == "user") {
          for (auto &each_item : m_user_scope_table) {
            for (auto &each_row : each_item.second) {
              if (each_row.var_owner == id && each_row.tag.empty()) {
                ret_vec.emplace_back(each_row.var_name, each_row.var_value);
              }
            }
          }
        } else {
          for (auto &each_item : m_contract_scope_table) {
            if (each_item.second.contract_id == id) {
              ret_vec.emplace_back(each_item.second.var_name, each_item.second.var_value);
            }
          }
        }
      }

      if (!ret_vec.empty())
        return ret_vec;


      // NOT IN SCOPE CACHE TABLE!

      nlohmann::json query;
      query["type"] = scope == "user" ? "user.scope.get" : "contract.scope.get";
      query["where"]["uid"] = id;
      query["where"]["notag"] = true;

      if (!name.empty() && name != "*")
        query["where"]["name"] = name;

      if (scope == "user") {
        return queryIfUserScopeAndParseData(query, id);
      } else {
        return queryIfContractScopeAndParseData(query, id);
      }
    }


  private:
    std::optional<UserScopeRecord> findUserScopeTableByName(std::string_view user_id, std::string_view var_name);

    std::optional<UserScopeRecord>
    findUserScopeTableByPid(std::string_view user_id, std::string_view var_name, std::string_view pid);

    std::vector<DataAttribute> queryIfAndParseData(nlohmann::json &query);

    std::vector<DataAttribute> queryIfUserCertAndParseData(nlohmann::json &query, const std::string &id);

    std::vector<DataAttribute> queryIfUserAttrAndParseData(nlohmann::json &query, const std::string &user_id);

    vector<DataAttribute>
    queryIfUserScopeAndParseData(nlohmann::json &query, std::string_view user_id, string_view pid = "",
                                 bool get_val = true);

    std::vector<DataAttribute> queryIfContractScopeAndParseData(nlohmann::json &query, const std::string &cid);

    std::pair<std::vector<std::string>, nlohmann::json> queryAndCache(nlohmann::json &query);

    inline std::string getEnumString(EnumGender gender_code);

    inline std::string getEnumString(EnumV v_code);

    inline std::string getEnumString(EnumAll enum_code);

    inline EnumGender getEnumGender(const std::string &enum_str);

    inline EnumV getEnumV(const std::string &enum_str);

    inline EnumAll getEnumAll(const std::string &enum_str);

    Datamap m_tx_datamap; // set of { keyword : value }
    std::string m_keyc_name{"KEYC"};
    std::unordered_map<std::string, nlohmann::json> m_query_cache;

    // TODO : replace cache tables to real cache

    std::map<std::string, std::vector<UserScopeRecord>> m_user_scope_table;
    std::map<std::string, ContractScopeRecord> m_contract_scope_table;

    std::map<std::string, UserAttributeRecord> m_user_attr_table;
    std::map<std::string, std::vector<UserCertRecord>> m_user_cert_table;

    std::function<nlohmann::json(nlohmann::json & )> m_read_storage_interface;
  };

}

#endif