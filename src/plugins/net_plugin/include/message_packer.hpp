#pragma once

#include "../../../lib/tethys-utils/src/hmac.hpp"
#include "../../../lib/tethys-utils/src/lz4_compressor.hpp"
#include "../config/include/message.hpp"

namespace tethys {
namespace net_plugin {

using namespace std;

class MessagePacker {
public:
  template <MACAlgorithmType T, typename = enable_if_t<T == MACAlgorithmType::NONE>>
  static string packMessage(OutNetMsg &out_msg) {
    return pack(out_msg);
  }

  template <MACAlgorithmType T, typename = enable_if_t<T == MACAlgorithmType::HMAC>>
  static string packMessage(OutNetMsg &out_msg, vector<uint8_t> &hmac_key) {
    auto packed_msg = pack(out_msg);
    auto hmac = Hmac::generateHMAC(packed_msg, hmac_key);
    return packed_msg + string(hmac.begin(), hmac.end());
  }

private:
  static string pack(OutNetMsg &out_msg) {
    string json_dump = out_msg.body.dump();

    auto vec_body = nlohmann::json::to_cbor(json_dump);
    string serialized_body(vec_body.begin(), vec_body.end());
    string header = makeHeader(serialized_body.size(), out_msg.type, SerializationAlgorithmType::CBOR);

    return (header + serialized_body);
  }

  static string makeHeader(int serialized_json_size, MessageType msg_type, SerializationAlgorithmType serialization_algo_type) {
    string serialized_header;

    serialized_header.push_back(IDENTIFIER);
    serialized_header.push_back(VERSION);
    serialized_header.push_back((uint8_t)msg_type);
    if (msg_type == MessageType::MSG_ACCEPT || msg_type == MessageType::MSG_REQ_SSIG)
      serialized_header.push_back((uint8_t)MACAlgorithmType::HMAC);
    else
      serialized_header.push_back((uint8_t)MACAlgorithmType::NONE);

    serialized_header.push_back((uint8_t)serialization_algo_type);
    serialized_header.push_back(NOT_USED);

    auto total_length = TypeConverter::integerToBytes(HEADER_LENGTH + serialized_json_size);
    serialized_header.insert(serialized_header.end(), begin(total_length), end(total_length));

    auto &world_id = app().getWorldId();
    serialized_header.insert(serialized_header.end(), world_id.begin(), world_id.end());

    auto &chain_id = app().getChainId();
    serialized_header.insert(serialized_header.end(), chain_id.begin(), chain_id.end());

    auto &my_id = app().getId();
    serialized_header.insert(serialized_header.end(), my_id.begin(), my_id.end());
    return serialized_header;
  }
};

} // namespace net_plugin
} // namespace tethys