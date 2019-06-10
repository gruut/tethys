#include "include/data_manager.hpp"

namespace tethys::tsce {

  void DataManager::attachReadInterface(std::function<nlohmann::json(nlohmann::json & )> &read_storage_interface) {
    m_read_storage_interface = read_storage_interface;
  }

  void DataManager::setKeyCurrencyName(std::string_view keyc_name) {
    m_keyc_name = keyc_name;
  }

  std::string DataManager::eval(std::string_view expr_view) {
    std::string expr(expr_view);
    return eval(expr);
  }

  std::string DataManager::eval(std::string &expr) {
    mt::trim(expr);

    if (expr.empty())
      return {};

    if (expr[0] != '$')
      return expr;

    auto eval_str = m_tx_datamap.get(expr);
    if (!eval_str)
      return {};

    return eval_str.value();
  }

  std::optional<std::string> DataManager::evalOpt(std::string_view expr_view) {
    std::string expr(expr_view);
    return evalOpt(expr);
  }

  std::optional<std::string> DataManager::evalOpt(std::string &expr) {
    mt::trim(expr);

    if (expr.empty())
      return {};

    if (expr[0] != '$')
      return expr;

    return m_tx_datamap.get(expr);
  }

  void DataManager::clear() {
    m_user_scope_table.clear();
    m_contract_scope_table.clear();
    m_tx_datamap.clear();
  }

  Datamap &DataManager::getDatamap() {
    return m_tx_datamap;
  }

  std::vector<DataAttribute> DataManager::getWorld() {
    nlohmann::json query;
    query["type"] = "world.get";
    return queryIfAndParseData(query);
  }

  std::vector<DataAttribute> DataManager::getChain() {
    nlohmann::json query;
    query["type"] = "chain.get";
    return queryIfAndParseData(query);
  }

  std::vector<DataAttribute> DataManager::getUserInfo(std::string_view user_id_) {
    if (user_id_.empty())
      return {};

    std::string user_id = eval(user_id_);

    auto it_tbl = m_user_attr_table.find(user_id);
    if (it_tbl != m_user_attr_table.end()) {
      std::vector<DataAttribute> ret_vec;
      ret_vec.emplace_back("register_day", it_tbl->second.register_day);
      ret_vec.emplace_back("register_code", it_tbl->second.register_code);
      ret_vec.emplace_back("gender", getEnumString(it_tbl->second.gender));
      ret_vec.emplace_back("isc_type", it_tbl->second.isc_type);
      ret_vec.emplace_back("isc_code", it_tbl->second.isc_code);
      ret_vec.emplace_back("location", it_tbl->second.location);
      ret_vec.emplace_back("age_limit", std::to_string(it_tbl->second.age_limit));
      return ret_vec;
    }

    nlohmann::json query;
    query["type"] = "user.info.get";
    query["where"]["uid"] = user_id;

    return queryIfUserAttrAndParseData(query, user_id);
  }

  int64_t DataManager::getUserKeyCurrency(std::string_view user_id_) {
    if (user_id_.empty())
      return {};

    std::string user_id = eval(user_id_);

    std::vector<DataAttribute> ret_vec;

    uint64_t keyc_amount = 0;

    std::string scope_key = user_id + m_keyc_name;
    auto it_tbl = m_user_scope_table.find(scope_key);
    if (it_tbl != m_user_scope_table.end() && !it_tbl->second.empty()) {
      for (auto &each_row : it_tbl->second) {
        if (each_row.tag.empty() && each_row.var_type == EnumAll::KEYC && each_row.var_name == m_keyc_name) {
          keyc_amount = mt::str2num<int64_t>(each_row.var_value);
          break;
        }
      }
    }

    if (!ret_vec.empty())
      return keyc_amount;

    nlohmann::json query;
    query["type"] = "user.scope.get";
    query["where"]["uid"] = user_id;
    query["where"]["name"] = m_keyc_name;
    query["where"]["type"] = getEnumString(EnumV::KEYC);
    query["where"]["notag"] = true;

    auto result = queryIfUserScopeAndParseData(query, user_id);

    if (!result.empty() && result[0].name == m_keyc_name) {
      keyc_amount = mt::str2num<uint64_t>(result[0].value);
    }

    return keyc_amount;

  }

  std::optional<UserScopeRecord>
  DataManager::findUserScopeTableByName(std::string_view user_id, std::string_view var_name) {

    std::cout << "called findUserScopeTableByName" << std::endl;

    std::string scope_key(user_id);
    scope_key.append(var_name);

    bool null_tag = false;
    bool found = false;

    UserScopeRecord ret_record;

    auto it_tbl = m_user_scope_table.find(scope_key);
    if (it_tbl != m_user_scope_table.end() && !it_tbl->second.empty()) {
      for (auto &each_row : it_tbl->second) {
        if (each_row.var_name == var_name) {
          found = true;

          if (each_row.tag.empty()) {
            null_tag = true;
            ret_record = each_row;
          }

          if (!null_tag)
            ret_record = each_row;
        }
      }
    }

    if (found)
      return ret_record;

    return std::nullopt;
  }

  std::optional<UserScopeRecord>
  DataManager::findUserScopeTableByPid(std::string_view user_id, std::string_view var_name, std::string_view pid) {
    std::cout << "called findUserScopeTableByPid" << std::endl;

    std::string scope_key(user_id);
    scope_key.append(var_name);

    //std::cout << scope_key << std::endl;

    auto it_tbl = m_user_scope_table.find(scope_key);
    if (it_tbl != m_user_scope_table.end() && !it_tbl->second.empty()) {
      for (auto &each_row : it_tbl->second) {
        if (each_row.pid == pid) {
          //std::cout << "found in m_user_scope_table" << std::endl;
          return each_row;
        }
      }
    }

    return std::nullopt;
  }

  std::vector<DataAttribute> DataManager::queryIfAndParseData(nlohmann::json &query) {
    std::cout << "called queryIfAndParseData" << std::endl;
    auto[result_name, result_data] = queryAndCache(query);

    std::vector<DataAttribute> ret_vec;

    if (!result_name.empty() && !result_data.empty()) {

      for (auto &each_row : result_data) {
        for (int i = 0; i < each_row.size(); ++i) { // last row will be selected
          std::string value = each_row[i].get<std::string>();
          ret_vec.emplace_back(result_name[i], value);
        }
      }
    }

    return ret_vec;
  }

  std::vector<DataAttribute> DataManager::queryIfUserCertAndParseData(nlohmann::json &query, const std::string &id) {
    std::cout << "called queryIfUserCertAndParseData" << std::endl;
    auto[result_name, result_data] = queryAndCache(query);

    std::vector<DataAttribute> ret_vec;

    if (!result_name.empty() && !result_data.empty()) {

      m_user_cert_table[id].clear();

      for (auto &each_row : result_data) {

        UserCertRecord buf_record;

        for (int i = 0; i < result_name.size(); ++i) {
          if (result_name[i] == "sn")
            buf_record.sn = each_row[i].get<std::string>();
          else if (result_name[i] == "after")
            buf_record.nvbefore = mt::str2num<uint64_t>(each_row[i].get<std::string>());
          else if (result_name[i] == "before")
            buf_record.nvafter = mt::str2num<uint64_t>(each_row[i].get<std::string>());
          else if (result_name[i] == "cert")
            buf_record.x509 = each_row[i].get<std::string>();

          std::string value = each_row[i].get<std::string>();

          ret_vec.emplace_back(result_name[i], value);
        }

        m_user_cert_table[id].emplace_back(buf_record);
      }

    }

    return ret_vec;
  }

  std::vector<DataAttribute>
  DataManager::queryIfUserAttrAndParseData(nlohmann::json &query, const std::string &user_id) {
    std::cout << "called queryIfUserAttrAndParseData" << std::endl;
    auto[result_name, result_data] = queryAndCache(query);

    std::vector<DataAttribute> ret_vec;

    if (!result_name.empty() && !result_data.empty()) {

      UserAttributeRecord buf_record;

      for (int i = 0; i < result_name.size(); ++i) {

        std::string col_data = result_data[0][i].get<std::string>();

        if (result_name[i] == "uid")
          buf_record.uid = col_data;
        else if (result_name[i] == "register_day")
          buf_record.register_day = col_data;
        else if (result_name[i] == "register_code")
          buf_record.register_code = col_data;
        else if (result_name[i] == "gender")
          buf_record.gender = static_cast<EnumGender>(mt::str2num<uint8_t>(col_data));
        else if (result_name[i] == "isc_type")
          buf_record.isc_type = col_data;
        else if (result_name[i] == "isc_code")
          buf_record.isc_code = col_data;
        else if (result_name[i] == "location")
          buf_record.location = col_data;
        else if (result_name[i] == "age_limit")
          buf_record.age_limit = mt::str2num<int>(col_data);

        ret_vec.emplace_back(result_name[i], col_data);
      }

      m_user_attr_table[user_id] = buf_record;

    }

    return ret_vec;
  }

  vector<DataAttribute>
  DataManager::queryIfUserScopeAndParseData(nlohmann::json &query, std::string_view user_id, string_view pid,
                                            bool get_val) {
    std::cout << "called queryIfUserScopeAndParseData" << std::endl;

    auto[result_name, result_data] = queryAndCache(query);

    std::vector<DataAttribute> ret_vec;

    if (!result_name.empty() && !result_data.empty()) {

      for (auto &each_row : result_data) {

        UserScopeRecord buf_record;

        for (int i = 0; i < result_name.size(); ++i) {

          std::string col_data = each_row[i].get<std::string>();

          if (result_name[i] == "var_name")
            buf_record.var_name = col_data;
          else if (result_name[i] == "var_value")
            buf_record.var_value = col_data;
          else if (result_name[i] == "var_type")
            buf_record.var_type = getEnumAll(col_data);
          else if (result_name[i] == "var_owner")
            buf_record.var_owner = col_data;
          else if (result_name[i] == "up_time")
            buf_record.up_time = mt::str2num<uint64_t>(col_data);
          else if (result_name[i] == "up_block")
            buf_record.up_block = mt::str2num<uint64_t>(col_data);
          else if (result_name[i] == "tag")
            buf_record.tag = col_data;
          else if (result_name[i] == "pid")
            buf_record.pid = col_data;
        }

        if (buf_record.var_name.empty())
          continue;

        if (buf_record.var_owner.empty())
          buf_record.var_owner = user_id;

        if (get_val) {
          if (pid.empty()) {
            if (buf_record.tag.empty())
              ret_vec.emplace_back(buf_record.var_name, buf_record.var_value);
          } else {
            if (buf_record.pid == pid)
              ret_vec.emplace_back(buf_record.var_name, buf_record.var_value);
          }
        }

        // update cache

        std::string scope_key(user_id);
        scope_key.append(buf_record.var_name);

        bool is_new = true;

        for (auto &user_scope_record : m_user_scope_table[scope_key]) {
          if (user_scope_record.pid == buf_record.pid) {
            user_scope_record.var_value = buf_record.var_value;
            is_new = false;
            break;
          }
        }

        if (is_new)
          m_user_scope_table[scope_key].emplace_back(buf_record);
      }
    }

    return ret_vec;
  }

  std::vector<DataAttribute>
  DataManager::queryIfContractScopeAndParseData(nlohmann::json &query, const std::string &cid) {
    std::cout << "called queryIfContractScopeAndParseData" << std::endl;
    auto[result_name, result_data] = queryAndCache(query);

    std::vector<DataAttribute> ret_vec;

    if (!result_name.empty() && !result_data.empty()) {

      for (auto &each_row : result_data) {

        ContractScopeRecord buf_record;

        for (int i = 0; i < result_name.size(); ++i) {

          std::string col_data = each_row[i].get<std::string>();

          if (result_name[i] == "var_name")
            buf_record.var_name = col_data;
          else if (result_name[i] == "var_value")
            buf_record.var_value = col_data;
          else if (result_name[i] == "var_type")
            buf_record.var_type = getEnumAll(col_data);
          else if (result_name[i] == "contract_id")
            buf_record.contract_id = col_data;
          else if (result_name[i] == "up_time")
            buf_record.up_time = mt::str2num<uint64_t>(col_data);
          else if (result_name[i] == "up_block")
            buf_record.up_block = mt::str2num<uint64_t>(col_data);
          else if (result_name[i] == "pid")
            buf_record.pid = col_data;
        }

        if (buf_record.var_name.empty())
          continue;

        if (buf_record.contract_id.empty())
          buf_record.contract_id = cid;

        std::string scope_key = cid + buf_record.var_name;
        m_contract_scope_table[scope_key] = buf_record; // insert or update

        ret_vec.emplace_back(buf_record.var_name, buf_record.var_value);
      }
    }

    return ret_vec;
  }

  std::pair<std::vector<std::string>, nlohmann::json> DataManager::queryAndCache(nlohmann::json &query) {
    std::cout << "called queryAndCache" << std::endl;
    std::string query_key = TypeConverter::toString(nlohmann::json::to_cbor(query));
    nlohmann::json query_result;

    std::cout << "> query = " << query << std::endl;

    auto it_cache = m_query_cache.find(query_key);
    if (it_cache != m_query_cache.end()) {
      query_result = (*it_cache).second;
    } else {
      query_result = m_read_storage_interface(query);
      m_query_cache[query_key] = query_result;
    }

    std::cout << "< result = " << query_result << std::endl;

    auto query_result_name = json::get<nlohmann::json>(query_result, "name");
    auto query_result_data = json::get<nlohmann::json>(query_result, "data");

    if (!query_result_name || !query_result_data)
      return {};

    std::vector<std::string> data_name;
    for (auto &each_name : query_result_name.value())
      data_name.push_back(each_name.get<std::string>());

    return {data_name, query_result_data.value()};
  }

  std::string DataManager::getEnumString(EnumGender gender_code) {
    return getEnumString(static_cast<EnumAll>(gender_code));
  }

  std::string DataManager::getEnumString(EnumV v_code) {
    return getEnumString(static_cast<EnumAll>(v_code));
  }

  std::string DataManager::getEnumString(EnumAll enum_code) {
    for (auto&[key, val] : INPUT_OPTION_TYPE_MAP) {
      if (val == enum_code)
        return key;
    }

    return {};
  }

  EnumGender DataManager::getEnumGender(const std::string &enum_str) {
    auto it = INPUT_OPTION_TYPE_MAP.find(enum_str);
    return (it == INPUT_OPTION_TYPE_MAP.end() ? EnumGender::NONE : static_cast<EnumGender>(it->second));
  }

  EnumV DataManager::getEnumV(const std::string &enum_str) {
    auto it = INPUT_OPTION_TYPE_MAP.find(enum_str);
    return (it == INPUT_OPTION_TYPE_MAP.end() ? EnumV::NONE : static_cast<EnumV>(it->second));
  }

  EnumAll DataManager::getEnumAll(const std::string &enum_str) {
    auto it = INPUT_OPTION_TYPE_MAP.find(enum_str);
    return (it == INPUT_OPTION_TYPE_MAP.end() ? EnumAll::NONE : it->second);
  }
}