#include "include/config.hpp"
#include "../../lib/tethys-utils/src/ecdsa.hpp"
#include <algorithm>
#include <cstring>
#include <array>
#include <botan-2/botan/bigint.h>
#include <botan-2/botan/ec_group.h>
#include <botan-2/botan/ecdsa.h>

#include "include/handler/signature_handler.hpp"

namespace tethys::tsce {
  bool SignatureHandler::evalue(tinyxml2::XMLElement *doc_node, DataManager &data_manager) {

    if (doc_node == nullptr)
      return false;

    auto signature_type = mt::c2s(doc_node->Attribute("type")); // GAMMA / ECDSA / .... default = GAMMA

    auto sig_node = doc_node->FirstChildElement("sig");
    auto pk_node = doc_node->FirstChildElement("pk");
    auto text_node = doc_node->FirstChildElement("text");
    if (sig_node == nullptr || pk_node == nullptr || text_node == nullptr)
      return false;

    std::string sig_val = mt::c2s(sig_node->Attribute("value"));
    std::string sig_contents_type = mt::c2s(sig_node->Attribute("type"));
    if (sig_val.empty() || sig_val[0] != '$')
      return false;

    std::string pk_val = mt::c2s(sig_node->Attribute("value"));
    std::string pk_type = mt::c2s(sig_node->Attribute("type")); // PEM / ENCODED-PK ...
    if (pk_val.empty())
      return false;
    else if (pk_val[0] == '$') {
      auto pk_val_temp = data_manager.get(pk_val);
      if (!pk_val_temp.has_value())
        return false;
      pk_val = pk_val_temp.value();
    }

    BytesBuilder bytes_builder;
    auto txt_val_nodes = XmlTool::parseChildrenFromNoIf(text_node, "val");
    for (auto &each_node : txt_val_nodes) {
      std::string val_value = mt::c2s(each_node->Attribute("value"));
      std::string val_type = mt::c2s(each_node->Attribute("type"));

      if (val_value.empty())
        continue;
      if (val_value[0] == '$') {
        auto temp_val = data_manager.get(val_value);
        if (!temp_val.has_value())
          continue;
        val_value = temp_val.value();
      }
      appendData(val_type, val_value, bytes_builder);
    }
    return verifySig(signature_type, sig_val, pk_type, pk_val, bytes_builder.getString());
  }

  void SignatureHandler::appendData(const std::string &type, const std::string &data, BytesBuilder &bytes_builder) {
    auto it = INPUT_OPTION_TYPE_MAP.find(type);
    EnumAll var_type;
    var_type = it != INPUT_OPTION_TYPE_MAP.end() ? it->second : EnumAll::NONE;

    switch (var_type) {
      case EnumAll::INT:
      case EnumAll::PINT:
      case EnumAll::NINT:
      case EnumAll::DEC: {
        bytes_builder.appendDec(data);
        break;
      }
      case EnumAll::FLOAT: {
        // TODO : need `appendFloat` function
        std::array<uint8_t, 4> bytes_arr{};
        auto float_val = std::stof(data);
        std::memcpy(bytes_arr.data(), &float_val, 4);
        bytes_builder.append(bytes_arr);
        break;
      }
      case EnumAll::BOOL: {
        vector<uint8_t> bool_data;
        if (std::all_of(data.begin(), data.end(), ::isdigit)) {
          auto num = std::stoi(data);
          (num > 0) ? bool_data.push_back(1) : bool_data.push_back(0);
        } else {
          std::string bool_str = mt::toLower(data);
          std::istringstream iss(bool_str);
          bool b;
          iss >> std::boolalpha >> b;
          b ? bool_data.push_back(1) : bool_data.push_back(0);
        }
        bytes_builder.append(bool_data);
        break;
      }
      case EnumAll::TINYTEXT:
      case EnumAll::TEXT:
      case EnumAll::LONGTEXT: {
        bytes_builder.append(data);
        break;
      }
      case EnumAll::DATETIME: {
        auto t = mt::timestr2timestamp(data);
        bytes_builder.appendDec(t);
        break;
      }
      case EnumAll::DATE: {
        auto t = mt::timestr2timestamp(data);
        bytes_builder.appendDec(t);
        break;
      }
      case EnumAll::BIN: {
        //TODO : need `appendBin` function
        int len = data.length() / 8;
        vector<uint8_t> temp_vec;
        for (int i = 0; i < len; i++) {
          auto one_byte = (uint8_t) stoi("0b" + string(data.substr(i * 8, 8)), nullptr, 2);
          temp_vec.push_back(one_byte);
        }
        bytes_builder.append(temp_vec);
        break;
      }
      case EnumAll::HEX: {
        bytes_builder.appendHex(data);
        break;
      }
      case EnumAll::BASE58: {
        bytes_builder.appendBase<58>(data);
        break;
      }
      case EnumAll::BASE64: {
        bytes_builder.appendBase<64>(data);
        break;
      }
      case EnumAll::ENUMV: {
        int val;
        if (data == "KEYC")
          val = static_cast<int>(EnumV::KEYC);
        else if (data == "FIAT")
          val = static_cast<int>(EnumV::FIAT);
        else if (data == "COIN")
          val = static_cast<int>(EnumV::COIN);
        else if (data == "XCOIN")
          val = static_cast<int>(EnumV::XCOIN);
        else
          val = static_cast<int>(EnumV::MILE);
        bytes_builder.appendDec(val);
        break;
      }
      case EnumAll::ENUMGENDER: {
        int val;
        if (data == "MALE")
          val = static_cast<int>(EnumGender::MALE);
        else if (data == "FEMALE")
          val = static_cast<int>(EnumGender::FEMALE);
        else
          val = static_cast<int>(EnumGender::OTHER);
        bytes_builder.appendDec(val);
        break;
      }
      case EnumAll::PEM:
      case EnumAll::ENUMALL:
      case EnumAll::CONTRACT:
      case EnumAll::XML: {
        //TODO : ???????
        break;
      }
      default:
        break;
    }
  }

  bool
  SignatureHandler::verifySig(std::string_view signature_type, const std::string &signature, std::string_view pk_type,
                              const string &pk_or_pem, const std::string &msg) {
    if (pk_type == "PEM") {
      /*
      if (signature_type.empty() || signature_type == "GAMMA") {
        AGS ags;
        auto pub_key =
            ags.getPublicKeyFromPem(pk_or_pem); // currently, `getPublicKeyFromPem` function is private member. it should be public one.
        return ags.verify(pub_key, msg, signature);
      } else if (signature_type == "ECDSA") {
        std::vector<uint8_t> sig_vec(signature.begin(), signature.end());
        return ECDSA::doVerify(pk_or_pem, msg, sig_vec);
      } else { //TODO: need `RSA` verification
        return false;
      }
       */

      return true;

    } else {
      if (signature_type.empty() || signature_type == "GAMMA") {
        /*
        AGS ags;
        return ags.verify(pk_or_pem, msg, signature);
         */
        return true;
      } else if (signature_type == "ECDSA") {
        try {
          std::vector<uint8_t> vec_x(pk_or_pem.begin() + 1, pk_or_pem.begin() + pk_or_pem.size() / 2 + 1);
          std::vector<uint8_t> vec_y(pk_or_pem.begin() + pk_or_pem.size() / 2 + 1, pk_or_pem.end());
          auto point_x = Botan::BigInt::decode(vec_x);
          auto point_y = Botan::BigInt::decode(vec_y);
          Botan::EC_Group group_domain("secp256k1");

          Botan::ECDSA_PublicKey public_key(group_domain, group_domain.point(point_x, point_y));
          std::vector<uint8_t> msg_vec(msg.begin(), msg.end());
          std::vector<uint8_t> sig_vec(signature.begin(), signature.end());
          return ECDSA::doVerify(public_key, msg_vec, sig_vec);
        } catch (...) {
          return false;
        }
      } else if (signature_type == "RSA") {
        //TODO: need `RSA` verification
        return false;
      } else {
        return false;
      }
    }
  }
}
