#ifndef TETHYS_SCE_INPUT_HANDLER_HPP
#define TETHYS_SCE_INPUT_HANDLER_HPP

#include "../config.hpp"
#include "../data_manager.hpp"
#include "../datamap.hpp"
#include "../xml_tool.hpp"

#include <regex>

namespace tethys::tsce {
constexpr int INT_LENGTH = 17;
constexpr long long MAX_INT = 9007199254740991;
constexpr long long MIN_INT = -9007199254740991;
constexpr int TINYTEXT_LEN = 255;
constexpr int TEXT_LEN = 65535;
constexpr int MEDIUMTEXT_LEN = 16777215;
const auto BOOL_REGEX = "^([Tt][Rr][Uu][Ee]|[Ff][Aa][Ll][Ss][Ee])$";
const auto DATE_REGEX = "([12]\\d{3}-(0[1-9]|1[0-2])-(0[1-9]|[12]\\d|3[01]))";
const auto DATE_TIME_REGEX =
    "(19|20)[0-9]{2}-(0[0-9]|1[0-2])-(0[1-9]|([12][0-9]|3[01]))T([01][0-9]|2[0-3]):[0-5][0-9]:[0-5][0-9](\\+|-)[0-1][0-9]:[00]";
const auto BASE64_REGEX = "^(?:[A-Za-z0-9+/]{4})*(?:[A-Za-z0-9+/]{3}=|[A-Za-z0-9+/]{2}==)?$";
const auto BASE58_REGEX = "^[A-HJ-NP-Za-km-z1-9]*$";
const auto BIN_REGEX = "^[0-1]*$";

struct InputOption {
  std::string key;
  std::string type;
  std::string validation;

  InputOption(std::string key_, std::string type_, std::string validation_)
      : key(std::move(key_)), type(std::move(type_)), validation(std::move(validation_)) {}
};

class InputHandler {
public:
  InputHandler() = default;

  bool parseInput(nlohmann::json &input_json, tinyxml2::XMLElement *input_node, DataManager &data_manager);

private:
  template <typename S = std::string>
  bool isValidValue(S &&value, S &&type, S &&validation) {
    auto it = INPUT_OPTION_TYPE_MAP.find(type);
    auto var_type = (it == INPUT_OPTION_TYPE_MAP.end() ? EnumAll::TEXT : it->second); // default is TEXT
    try {
      switch (var_type) {
      case EnumAll::INT: {
        if (value.length() > INT_LENGTH)
          return false;
        int64_t val = mt::str2num<int64_t>(value);
        if (!(val >= MIN_INT && val <= MAX_INT))
          return false;
        break;
      }
      case EnumAll::PINT: {
        if (value.length() > INT_LENGTH)
          return false;
        uint64_t val = mt::str2num<uint64_t>(value);
        if (!(val >= 1 && val <= MAX_INT))
          return false;
        break;
      }
      case EnumAll::NINT: {
        if (value.length() > INT_LENGTH)
          return false;
        int64_t val = mt::str2num<int64_t>(value);
        if (!(val >= MIN_INT && val <= -1))
          return false;
        break;
      }
      case EnumAll::FLOAT: {
        try {
          auto f = std::stold(value);
        } catch (...) {
          return false;
        }
        break;
      }
      case EnumAll::BOOL: {
        std::regex rgx(BOOL_REGEX);
        if (!std::regex_match(value, rgx))
          return false;
        //        if(mt::str2num<int64_t>(value) < 0 )
        //          return false;
        break;
      }
      case EnumAll::TINYTEXT: {
        if (value.length() > TINYTEXT_LEN)
          return false;
        break;
      }
      case EnumAll::TEXT: {
        if (value.length() > TEXT_LEN)
          return false;
        break;
      }
      case EnumAll::MEDIUMTEXT: {
        if (value.length() > MEDIUMTEXT_LEN)
          return false;
        break;
      }
      case EnumAll::DATE: {
        std::regex rgx(DATE_REGEX);
        if (!std::regex_match(value, rgx))
          return false;
        break;
      }
      case EnumAll::DATETIME: {
        std::regex rgx(DATE_TIME_REGEX);
        if (!std::regex_match(value, rgx))
          return false;
        break;
      }
      case EnumAll::BIN: {
        std::regex rgx(BIN_REGEX);
        if (!std::regex_match(value, rgx))
          return false;
        break;
      }
      case EnumAll::DEC: {
        if (!std::all_of(value.begin(), value.end(), ::isdigit))
          return false;
        break;
      }
      case EnumAll::HEX: {
        if (!std::all_of(value.begin(), value.end(), ::isxdigit))
          return false;
        break;
      }
      case EnumAll::BASE58: {
        std::regex rgx(BASE58_REGEX);
        if (!std::regex_match(value, rgx))
          return false;
        break;
      }
      case EnumAll::BASE64: {
        std::regex rgx(BASE64_REGEX);
        if (!std::regex_match(value, rgx))
          return false;
        break;
      }
      case EnumAll::ENUMV: {
        if (!mt::inArray(value, {"KEYC", "FIAT", "COIN", "XCOIN"}))
          return false;
        break;
      }
      case EnumAll::ENUMGENDER: {
        if (!mt::inArray(value, {"MALE", "FEMALE", "OTHER"}))
          return false;
        break;
      }
      case EnumAll::ENUMALL: {
        // TODO : handle `ENUMALL` type
        break;
      }
      case EnumAll::PEM: {
        string begin_str = "-----BEGIN CERTIFICATE-----";
        string end_str = "-----END CERTIFICATE-----";
        auto found1 = value.find(begin_str);
        auto found2 = value.find(end_str);
        if (found1 == string::npos || found2 == string::npos) {
          return false;
        }
        auto content_len = value.length() - begin_str.length() - end_str.length();
        auto content = value.substr(begin_str.length(), content_len);
        std::regex rgx(BASE64_REGEX);
        if (!std::regex_match(value, rgx))
          return false;
        break;
      }
      case EnumAll::XML:
      case EnumAll::CONTRACT: {

        tinyxml2::XMLDocument tmp_doc;

        tmp_doc.Parse(value.c_str());

        if (tmp_doc.Error())
          return false;

        if (var_type == EnumAll::XML)
          break;
        else {

          // TODO : enhance contract checking

          std::string check_begin = "<contract";
          std::string check_end = "</contract>";
          auto begin_str = value.substr(0, check_begin.length());
          auto end_str = value.substr(value.length() - check_end.length(), check_end.length());
          if (check_begin != begin_str || check_end != end_str)
            return false;
          break;
        }
      }
      default:
        return false;
      }

      if (!validation.empty()) {
        std::regex rgx(validation);
        return std::regex_match(value, rgx);
      }

      return true;
    } catch (...) { // when exceptions occur //ex) stoll, stod, stoi ...
      return false;
    }
  }
};

} // namespace tethys::tsce

#endif
