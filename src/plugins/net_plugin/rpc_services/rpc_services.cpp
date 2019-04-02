#include "include/rpc_services.hpp"

#include "../../../../lib/gruut-utils/src/time_util.hpp"
#include "../../channel_interface/include/channel_interface.hpp"
#include "../../../../lib/gruut-utils/src/lz4_compressor.hpp"
#include "../include/msg_handler.hpp"
#include "application.hpp"

#include <cstring>
#include <exception>
#include <future>
#include <thread>

namespace gruut {
namespace net_plugin {

using namespace appbase;
using namespace std;

void OpenChannel::proceed() {

  switch (m_receive_status) {
  case RpcCallStatus::CREATE: {
    m_receive_status = RpcCallStatus::READ;
    m_context.AsyncNotifyWhenDone(this);
    m_service->RequestOpenChannel(&m_context, &m_stream, m_completion_queue, m_completion_queue, this);
  } break;

  case RpcCallStatus::READ: {
    new OpenChannel(m_service, m_completion_queue, m_signer_table);
    m_receive_status = RpcCallStatus::PROCESS;
    m_stream.Read(&m_request, this);
  } break;

  case RpcCallStatus::PROCESS: {
    m_signer_id_b64 = m_request.sender();
    m_receive_status = RpcCallStatus::WAIT;
    m_signer_table->setReplyMsg(m_signer_id_b64, &m_stream, this);

  } break;

  case RpcCallStatus::WAIT: {
    if (m_context.IsCancelled()) {
      MessageHandler msg_handler;
      auto internal_msg = msg_handler.genInternalMsg(MessageType::MSG_LEAVE, m_signer_id_b64);
      if (internal_msg.has_value()) {
        m_signer_table->eraseRpcInfo(m_signer_id_b64);
        auto &in_msg_channel = app().getChannel<incoming::channels::network::channel_type>();
        in_msg_channel.publish(internal_msg.value());
      }
      delete this;
    }
  } break;

  default:
    break;
  }
}

void GeneralService::proceed() {
  switch (m_receive_status) {
  case RpcCallStatus::CREATE: {
    m_receive_status = RpcCallStatus::PROCESS;
    m_service->RequestGeneralService(&m_context, &m_request, &m_responder, m_completion_queue, m_completion_queue, this);
  } break;

  case RpcCallStatus::PROCESS: {
    new GeneralService(m_service, m_completion_queue, m_routing_table, m_broadcast_check_table);
    Status rpc_status;

    try {
      std::string packed_msg = m_request.message();
      // Forwarding message to other nodes
      if (m_request.broadcast()) {
        std::string msg_id = m_request.message_id();

        if (m_broadcast_check_table->count(msg_id) > 0) {
          m_reply.set_status(grpc_general::MsgStatus_Status_DUPLICATED);
          m_reply.set_message("duplicate message");
          m_receive_status = RpcCallStatus::FINISH;
          m_responder.Finish(m_reply, rpc_status, this);
          break;
        }
        uint64_t now = TimeUtil::nowBigInt();
        m_broadcast_check_table->insert({msg_id, now});

        RequestMsg request;
        request.set_message(packed_msg);
        request.set_broadcast(true);

        ClientContext context;
        std::chrono::time_point deadline = std::chrono::system_clock::now() + GENERAL_SERVICE_TIMEOUT;
        context.set_deadline(deadline);

        MsgStatus msg_status;

        for (auto &bucket : *m_routing_table) {
          if (!bucket.empty()) {
            auto nodes = bucket.selectRandomAliveNodes(PARALLELISM_ALPHA);
            for (auto &n : nodes) {
              auto stub = GruutGeneralService::NewStub(n.getChannelPtr());
              stub->GeneralService(&context, request, &msg_status);
            }
          }
        }
      }

      auto input_data = parseMessage(packed_msg, rpc_status);

      if (rpc_status.ok()) {
        m_reply.set_status(grpc_general::MsgStatus_Status_SUCCESS); // SUCCESS
        m_reply.set_message("OK");
        auto &in_msg_channel = app().getChannel<incoming::channels::network::channel_type>();
        in_msg_channel.publish(input_data.value());
      } else {
        m_reply.set_status(grpc_general::MsgStatus_Status_INVALID); // INVALID
        m_reply.set_message(rpc_status.error_message());
      }
    } catch (std::exception &e) { // INTERNAL
      m_reply.set_status(grpc_general::MsgStatus_Status_INTERNAL);
      m_reply.set_message("Merger internal error");
    }
    m_receive_status = RpcCallStatus::FINISH;
    m_responder.Finish(m_reply, rpc_status, this);

  } break;

  default: { delete this; } break;
  }
}

optional<InNetMsg> GeneralService::parseMessage(string &packed_msg, Status &return_rpc_status) {
  string raw_header(packed_msg.begin(), packed_msg.begin() + HEADER_LENGTH);
  auto msg_header = parseHeader(raw_header);
  if (!validateMsgFormat(msg_header)) {
    return_rpc_status = Status(StatusCode::INVALID_ARGUMENT, "Bad request (Invalid parameter)");
    return {};
  }

  auto body_size = convertU8ToU32BE(msg_header->total_length);
  string msg_raw_body(packed_msg.begin() + HEADER_LENGTH, packed_msg.begin() + HEADER_LENGTH + body_size);

  if (msg_header->mac_algo_type == MACAlgorithmType::HMAC) {
    vector<uint8_t> hmac(packed_msg.begin() + HEADER_LENGTH + body_size, packed_msg.end());

    // TODO : verify HMAC
  }

  auto json_body = getJson(msg_header->compression_algo_type, msg_raw_body);

  if (!JsonValidator::validateSchema(json_body, msg_header->message_type)) {
    return_rpc_status = Status(StatusCode::INVALID_ARGUMENT, "Bad request (Json schema error)");
    return {};
  }

  return_rpc_status = Status::OK;
  return InNetMsg{msg_header->message_type, json_body, msg_header->sender_id};
}

MessageHeader* GeneralService::parseHeader(string &raw_header) {
  auto msg_header = reinterpret_cast<MessageHeader *>(&raw_header);
  return msg_header;
}

bool GeneralService::validateMsgFormat(MessageHeader *header) {
  bool check = header->identifier == IDENTIFIER;
  if (header->mac_algo_type == MACAlgorithmType::HMAC) {
    check &= (header->message_type == MessageType::MSG_SUCCESS || header->message_type == MessageType::MSG_SSIG);
  }

  return check;
}

int GeneralService::convertU8ToU32BE(array<uint8_t, MSG_LENGTH_SIZE> &len_bytes) {
  return static_cast<int>(len_bytes[0] << 24 | len_bytes[1] << 16 | len_bytes[2] << 8 | len_bytes[3]);
}

nlohmann::json GeneralService::getJson(CompressionAlgorithmType comperssion_type, string &raw_body) {
  nlohmann::json unpacked_body;
  if (!raw_body.empty()) {
    switch (comperssion_type) {
      case CompressionAlgorithmType::LZ4: {
        string origin_data = LZ4Compressor::decompressData(raw_body);
      }
      case CompressionAlgorithmType::NONE: {
        unpacked_body = json::parse(raw_body);
      }
      default:
        break;
    }
  }
  return unpacked_body;
}

void FindNode::proceed() {
  switch (m_receive_status) {
  case RpcCallStatus::CREATE: {
    m_receive_status = RpcCallStatus::PROCESS;
    m_service->RequestFindNode(&m_context, &m_request, &m_responder, m_completion_queue, m_completion_queue, this);
  } break;

  case RpcCallStatus::PROCESS: {
    new FindNode(m_service, m_completion_queue, m_routing_table);

    std::string target = m_request.target_id();
    uint64_t time_stamp = m_request.time_stamp();

    auto neighbor_list = m_routing_table->findNeighbors(Hash<160>::sha1(target), PARALLELISM_ALPHA);

    for (auto &n : neighbor_list) {
      Neighbors_node *node = m_reply.add_neighbors();

      node->set_address(n.getEndpoint().address);
      node->set_port(n.getEndpoint().port);
      node->set_node_id(n.getId());
    }
    m_reply.set_time_stamp(TimeUtil::nowBigInt());

    Status rpc_status = Status::OK;
    m_receive_status = RpcCallStatus::FINISH;
    m_responder.Finish(m_reply, Status::OK, this);
  } break;

  default: { delete this; } break;
  }
}

} // namespace net_plugin
} // namespace gruut