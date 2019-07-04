#ifndef TETHYS_SCE_DUMMY_STORAGE_HPP
#define TETHYS_SCE_DUMMY_STORAGE_HPP

#include "../../../../lib/json/include/json.hpp"

using namespace tethys;

std::function<nlohmann::json(nlohmann::json &)> read_storage_interface = [](nlohmann::json &query) {
  // clang-format off on
  nlohmann::json result = R"({
        "name": [],
        "data": []
    })"_json;

  std::string query_type = json::get<std::string>(query, "type").value_or("");

  if (query_type == "world.get") {

    result["name"] = {"world_id",  "created_time",        "creator_id",   "creator_pk",  "authority_id",         "authority_pk",
                      "keyc_name", "keyc_initial_amount", "allow_mining", "mining_rule", "allow_anonymous_user", "join_fee"};

    nlohmann::json record = {"TETHYS19",
                             "0",
                             "5g9CMGLSXbNAKJMbWqBNp7rm78BJCMKhLzZVukBNGHSF",
                             "",
                             "5g9CMGLSXbNAKJMbWqBNp7rm78BJCMKhLzZVukBNGHSF",
                             "",
                             "THY",
                             "100000000000",
                             "false",
                             "",
                             "true",
                             "10"};

    result["data"].emplace_back(record);

  } else if (query_type == "chain.get") {

    result["name"] = {"chain_id",     "created_time", "creator_id",          "creator_pk", "allow_custom_contract",
                      "allow_oracle", "allow_tag",    "allow_heavy_contract"};

    nlohmann::json record = {"SEOUL@KR", "1", "5g9CMGLSXbNAKJMbWqBNp7rm78BJCMKhLzZVukBNGHSF", "", "false", "false", "false", "false"};

    result["data"].emplace_back(record);

  } else if (query_type == "contract.get") {

    std::string test_contract = R"(<contract>
  <head>
    <cid>VALUE-TRANSFER::5g9CMGLSXbNAKJMbWqBNp7rm78BJCMKhLzZVukBNGHSF::SEOUL@KR::TETHYS19</cid>
    <after>2018-01-01T00:00:00+09:00</after>
    <desc>Official standardcontract for transfering v-type variable</desc>
  </head>
  <body>
    <input>
      <option name="amount" type="PINT" desc="amount of v-type variable for transfer" />
      <option name="unit" type="TINYTEXT" desc="unit type of v-type variable for transfer" />
      <option name="pid" type="BASE64" desc="id of v-type variable for transfer" />
      <option name="tag" type="XML" desc="condition for use of this v-type value" />
    </input>
    <set type="v.transfer" from="user">
      <option name="to" value="$receiver" />
      <option name="amount" value="$0.amount" />
      <option name="unit" value="$0.unit" />
      <option name="pid" value="$0.pid" />
      <option name="tag" value="$0.tag" />
    </set>
  </body>
  <fee>
    <pay from="user" value="$fee" />
  </fee>
</contract>)";

    result["name"].emplace_back("contract");

    nlohmann::json record = nlohmann::json::array();
    record.emplace_back(test_contract);

    result["data"].emplace_back(record);

  } else if (query_type == "user.info.get") {

    result["name"] = {"register_day", "register_code", "gender", "isc_type", "isc_code", "location", "age_limit"};

    nlohmann::json record = {"1980-08-15", "", "MALE", "", "", "", ""};

    result["data"].emplace_back(record);
  } else if (query_type == "user.scope.get") {

    result["name"] = {"var_name", "var_value", "var_type", "up_time", "up_block", "tag", "pid"};

    if (json::get<std::string>(query["where"], "name").value_or("") == "THY" &&
        json::get<std::string>(query["where"], "type").value_or("") == "KEYC" && json::get<bool>(query["where"], "notag").value_or(false)) {
      nlohmann::json record = {"THY", "1000", "KEYC", "0", "0", "", "VGhlIHF1aWNrIGJyb3duIGZveCBqdW1wcyBvdmVyIDE="};
      result["data"].emplace_back(record);
    }

    if (json::get<std::string>(query["where"], "pid").value_or("") == "8CJ8YhBwwgNGKAdzGl1qkKstJi+rUQ7ow8gMHIF3RHU=") {
      std::string tag = R"(<tag>
  <info>
    <name>Fafa's Love</name>
    <cword>Flower</cword>
  </info>
  <update>
    <time>
      <after>2019-05-30T13:44:19+09:00</after>
    </time>
  </update>
</tag>)";
      nlohmann::json record = {"THY", "90", "KEYC", "0", "0", tag, "8CJ8YhBwwgNGKAdzGl1qkKstJi+rUQ7ow8gMHIF3RHU="};
      result["data"].emplace_back(record);
    }

  } else {
  }
  // clang-format on
  return result;
};

#endif // TETHYS_SCE_DUMMY_STORAGE_HPP
