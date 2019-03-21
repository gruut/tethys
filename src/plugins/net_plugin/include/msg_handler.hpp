#pragma once
#include "../../../../lib/json/include/json.hpp"
#include "../config/include/message.hpp"
#include "../config/include/network_config.hpp"
#include "../../../gruut-utils/src/lz4_compressor.hpp"
#include "channel_interface.hpp"

namespace gruut {
namespace net_plugin {

class MessageHandler {

public:
  InNetMsg unpackMsg(std::string &packed_msg, grpc::Status &return_rpc_status) {
    using namespace grpc;

    InNetMsg msg_entry;

    if(packed_msg.size() < HEADER_LENGTH){
	  return_rpc_status = Status(StatusCode::INVALID_ARGUMENT, "Wrong Message (MessageHandler::unpacked)");
      return msg_entry;
    }

    std::string raw_header(packed_msg.begin(), packed_msg.begin() + HEADER_LENGTH);

    auto msg_header = parseHeader(raw_header);

    if(!validateMsgFormat(msg_header)){
	  return_rpc_status = Status(StatusCode::INVALID_ARGUMENT, "Wrong Message (MessageHandler::unpacked)");
      return msg_entry;
    }

    auto body_size = convertU8ToU32BE(msg_header->total_length);
	std::string msg_raw_body(packed_msg.begin() + HEADER_LENGTH, packed_msg.begin() + HEADER_LENGTH + body_size);

	if(msg_header->mac_algo_type == MACAlgorithmType::HMAC){
      std::vector<uint8_t> hmac(packed_msg.begin() + HEADER_LENGTH + body_size, packed_msg.end());

      //TODO : verify HMAC
    }

	auto json_body = getJson(msg_header->compression_algo_type, msg_raw_body);

	//TODO : validate json schema

	return_rpc_status = Status::OK;
	msg_entry.body = json_body;
	msg_entry.type = msg_header->message_type;
	msg_entry.sender_id = msg_header->sender_id;

	return msg_entry;
  }

  std::string packMsg(OutNetMsg &output_msg) {

    std::string json_dump = output_msg.body.dump();
    std::string compressed_body = LZ4Compressor::compressData(json_dump);
    std::string header = makeHeader(compressed_body.size(), output_msg.type, CompressionAlgorithmType::NONE);

    return header + compressed_body;
  }

private:
  MessageHeader* parseHeader(std::string &raw_header) {
	auto msg_header = reinterpret_cast<MessageHeader *>(&raw_header);
	return msg_header;
  }

  std::string makeHeader(int compressed_json_size, MessageType msg_type,
						   CompressionAlgorithmType compression_algo_type) {

	MessageHeader msg_header;
	msg_header.identifier = IDENTIFIER;
	msg_header.version = VERSION;
	msg_header.message_type = msg_type;
	if(msg_type == MessageType::MSG_ACCEPT || msg_type == MessageType::MSG_REQ_SSIG)
	  msg_header.mac_algo_type = MACAlgorithmType::HMAC;
	else
	  msg_header.mac_algo_type = MACAlgorithmType::NONE;

	msg_header.compression_algo_type = compression_algo_type;
	msg_header.dummy = NOT_USED;

	int total_length = HEADER_LENGTH + compressed_json_size;

	for(int i = MSG_LENGTH_SIZE; i > 0; i--){
	  msg_header.total_length[i] |= total_length;
	  total_length >>= 8;
	}
	msg_header.total_length[0] |= total_length;

	std::copy(std::begin(LOCAL_CHAIN_ID), std::end(LOCAL_CHAIN_ID), std::begin(msg_header.local_chain_id));
	std::copy(std::begin(MY_ID), std::end(MY_ID), std::begin(msg_header.sender_id));

	auto header_ptr = reinterpret_cast<uint8_t *>(&msg_header);
	auto serialized_header = std::string(header_ptr, header_ptr + sizeof(msg_header));

	return serialized_header;
  }

  bool validateMsgFormat(MessageHeader* header){
    bool check = header->identifier == IDENTIFIER;
  	if(header->mac_algo_type == MACAlgorithmType::HMAC) {
  	  check &= (header->message_type == MessageType::MSG_SUCCESS ||
  	  			header->message_type == MessageType::MSG_SSIG);
  	}
  	return check;
  }

  int convertU8ToU32BE(std::array<uint8_t, MSG_LENGTH_SIZE> &len_bytes){
	return static_cast<int>(len_bytes[0] << 24 | len_bytes[1] << 16 |
		len_bytes[2] << 8 | len_bytes[3]);
  }

  nlohmann::json getJson(CompressionAlgorithmType comperssion_type, std::string &raw_body){
    nlohmann::json unpacked_body;
    if(!raw_body.empty()){
      switch(comperssion_type){
      case CompressionAlgorithmType::LZ4:{
        std::string origin_data = LZ4Compressor::decompressData(raw_body);
      }
      case CompressionAlgorithmType::NONE:{
        unpacked_body = json::parse(raw_body);
      }
	  default:
	    break;
      }
    }
    return unpacked_body;
  }

};
}
}