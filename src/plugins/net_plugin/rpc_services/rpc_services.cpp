#include "include/rpc_services.hpp"
#include "../../../../lib/gruut-utils/src/hmac.hpp"
#include "../../../../lib/gruut-utils/src/lz4_compressor.hpp"
#include "../../../../lib/gruut-utils/src/time_util.hpp"
#include "../../../../lib/gruut-utils/src/type_converter.hpp"
#include "../../channel_interface/include/channel_interface.hpp"
#include "../include/id_mapping_table.hpp"
#include "../include/message_builder.hpp"
#include "../include/message_packer.hpp"
#include "../include/signer_pool_manager.hpp"
#include "application.hpp"

#include <boost/algorithm/string.hpp>

#include <algorithm>
#include <cstring>
#include <exception>
#include <future>
#include <regex>
#include <thread>

namespace gruut {
namespace net_plugin {

using namespace appbase;
using namespace std;

class MessageParser {
public:
  optional<InNetMsg> parseMessage(string_view packed_msg, Status &return_rpc_status) {
    const string raw_header(packed_msg.begin(), packed_msg.begin() + HEADER_LENGTH);
    auto msg_header = parseHeader(raw_header);
    if (!validateMsgHdrFormat(msg_header)) {
      return_rpc_status = Status(StatusCode::INVALID_ARGUMENT, "Bad request (Invalid parameter)");
      return {};
    }
    auto body_size = convertU8ToU32BE(msg_header.total_length) - HEADER_LENGTH;
    string msg_raw_body(packed_msg.begin() + HEADER_LENGTH, packed_msg.begin() + HEADER_LENGTH + body_size);
    auto json_body = getJson(msg_header.serialization_algo_type, msg_raw_body);

    if (!JsonValidator::validateSchema(json_body, msg_header.message_type)) {
      return_rpc_status = Status(StatusCode::INVALID_ARGUMENT, "Bad request (Json schema error)");
      return {};
    }

    return_rpc_status = Status::OK;

    InNetMsg in_msg;
    in_msg.type = msg_header.message_type;
    in_msg.body = json_body;
    in_msg.sender_id = TypeConverter::encodeBase<58>(msg_header.sender_id);

    return in_msg;
  }

private:
  MessageHeader parseHeader(string_view raw_header) {
    MessageHeader msg_header;

    int offset = 0;
    msg_header.identifier = raw_header[offset++];
    msg_header.version = raw_header[offset++];
    msg_header.message_type = (MessageType)raw_header[offset++];
    msg_header.mac_algo_type = (MACAlgorithmType)raw_header[offset++];
    msg_header.serialization_algo_type = (SerializationAlgorithmType)raw_header[offset++];
    msg_header.dummy = raw_header[offset++];

    copy(raw_header.begin() + offset, raw_header.begin() + offset + MSG_LENGTH_SIZE, msg_header.total_length.begin());
    offset += MSG_LENGTH_SIZE;
    copy(raw_header.begin() + offset, raw_header.begin() + offset + WORLD_ID_TYPE_SIZE, msg_header.world_id.begin());
    offset += WORLD_ID_TYPE_SIZE;
    copy(raw_header.begin() + offset, raw_header.begin() + offset + CHAIN_ID_TYPE_SIZE, msg_header.local_chain_id.begin());
    offset += CHAIN_ID_TYPE_SIZE;
    copy(raw_header.begin() + offset, raw_header.begin() + offset + SENDER_ID_TYPE_SIZE, msg_header.sender_id.begin());
    return msg_header;
  }

  bool validateMsgHdrFormat(MessageHeader &header) {
    bool check = header.identifier == IDENTIFIER;
    if (header.mac_algo_type == MACAlgorithmType::HMAC) {
      check &= (header.message_type == MessageType::MSG_SUCCESS || header.message_type == MessageType::MSG_SSIG);
    }
    return check;
  }

  int convertU8ToU32BE(array<uint8_t, MSG_LENGTH_SIZE> &len_bytes) {
    return static_cast<int>(len_bytes[0] << 24 | len_bytes[1] << 16 | len_bytes[2] << 8 | len_bytes[3]);
  }

  nlohmann::json getJson(SerializationAlgorithmType comperssion_type, string &raw_body) {
    nlohmann::json unpacked_body;
    if (!raw_body.empty()) {
      switch (comperssion_type) {
      case SerializationAlgorithmType::LZ4: {
        string origin_data = LZ4Compressor::decompressData(raw_body);
        unpacked_body = json::parse(origin_data);
        break;
      }
      case SerializationAlgorithmType::NONE: {
        unpacked_body = json::parse(raw_body);
        break;
      }
      default:
        break;
      }
    }
    return unpacked_body;
  }
};

class MessageHandler {
public:
  explicit MessageHandler(ServerContext *server_context, shared_ptr<IdMappingTable> table)
      : context(server_context), id_mapping_table(table) {}
  explicit MessageHandler(shared_ptr<SignerPoolManager> pool_manager) : signer_pool_manager(pool_manager) {}
  optional<OutNetMsg> operator()(InNetMsg &msg) {
    return handle_message(msg);
  }

private:
  optional<OutNetMsg> handle_message(InNetMsg &msg) {
    auto msg_type = msg.type;

    if (msg_type == MessageType::MSG_BLOCK || msg_type == MessageType::MSG_TX || msg_type == MessageType::MSG_REQ_BLOCK) {
      mapping_user_id_to_net_id(msg);
    }

    switch (msg.type) {
    case MessageType::MSG_TX:
      app().getChannel<incoming::channels::transaction::channel_type>().publish(msg.body);
      return {};
    case MessageType::MSG_JOIN:
    case MessageType::MSG_RESPONSE_1:
    case MessageType::MSG_SUCCESS:
      return signer_pool_manager->handleMsg(msg);
    case MessageType::MSG_SSIG:
      // TODO : ssig message must be sent to `block producer`
      return {};
    default:
      return {};
    }
  }

  void mapping_user_id_to_net_id(InNetMsg &msg) {
    auto b58_user_id = TypeConverter::encodeBase<58>(msg.sender_id);
    auto net_id = context->client_metadata().find("net_id")->second;
    id_mapping_table->mapId(b58_user_id, string(net_id.data()));
  }

  ServerContext *context;
  shared_ptr<IdMappingTable> id_mapping_table;
  shared_ptr<SignerPoolManager> signer_pool_manager;
};

void OpenChannelWithSigner::proceed() {
  switch (m_receive_status) {
  case RpcCallStatus::CREATE: {
    m_receive_status = RpcCallStatus::READ;
    m_context.AsyncNotifyWhenDone(this);
    m_service->RequestOpenChannel(&m_context, &m_stream, m_completion_queue, m_completion_queue, this);
  } break;

  case RpcCallStatus::READ: {
    new OpenChannelWithSigner(m_service, m_completion_queue, m_signer_conn_table, m_signer_pool_manager);
    m_receive_status = RpcCallStatus::PROCESS;
    m_stream.Read(&m_request, this);
  } break;

  case RpcCallStatus::PROCESS: {
    m_signer_id_b58 = m_request.sender();
    m_receive_status = RpcCallStatus::WAIT;
    m_signer_conn_table->setRpcInfo(m_signer_id_b58, &m_stream, this);

  } break;

  case RpcCallStatus::WAIT: {
    if (m_context.IsCancelled()) {
      m_signer_conn_table->eraseRpcInfo(m_signer_id_b58);
      m_signer_pool_manager->removeSigner(m_signer_id_b58);
      m_signer_pool_manager->removeTempSigner(m_signer_id_b58);
      // TODO: signer leave event should be notified to `Block producer`

      delete this;
    }
  } break;

  default:
    break;
  }
}

Status SignerService::verifyHMAC(string_view packed_msg, vector<uint8_t> &hmac_key) {
  auto hmac_str = packed_msg.substr(packed_msg.length() - 32);
  vector<uint8_t> hmac(hmac_str.begin(), hmac_str.end());
  auto raw_msg = string(packed_msg.substr(0, packed_msg.length() - 32));
  if (!Hmac::verifyHMAC(raw_msg, hmac, hmac_key)) {
    return Status(StatusCode::UNAUTHENTICATED, "Bad request (Wrong HMAC)");
  }
  return Status::OK;
}

void SignerService::proceed() {
  switch (m_receive_status) {
  case RpcCallStatus::CREATE: {
    m_receive_status = RpcCallStatus::PROCESS;
    m_service->RequestSignerService(&m_context, &m_request, &m_responder, m_completion_queue, m_completion_queue, this);
  } break;

  case RpcCallStatus::PROCESS: {
    new SignerService(m_service, m_completion_queue, m_signer_pool_manager);
    Status rpc_status;
    try {
      string packed_msg = m_request.message();
      MessageParser msg_parser;
      auto input_data = msg_parser.parseMessage(packed_msg, rpc_status);
      if (input_data.has_value()) {
        auto msg_type = input_data.value().type;
        if (msg_type == MessageType::MSG_SSIG || msg_type == MessageType::MSG_SUCCESS) {
          auto signer_id_b58 = input_data.value().sender_id;
          auto hmac_key = m_signer_pool_manager->getHmacKey(signer_id_b58);
          if (hmac_key.has_value())
            rpc_status = verifyHMAC(packed_msg, hmac_key.value());
          else
            rpc_status = Status(StatusCode::UNAUTHENTICATED, "Bad request (cannot find hmac key)");
        }
        if (rpc_status.ok()) {
          MessageHandler msg_handler(m_signer_pool_manager);
          auto reply_msg = msg_handler(input_data.value());

          if (reply_msg.has_value()) {
            auto reply_msg_type = reply_msg.value().type;
            string reply_packed_msg;
            if (reply_msg_type == MessageType::MSG_ACCEPT) {
              auto receiver_id_b58 = reply_msg.value().receivers[0];
              auto hmac_key = m_signer_pool_manager->getHmacKey(receiver_id_b58);
              if (hmac_key.has_value())
                reply_packed_msg = MessagePacker::packMessage<MACAlgorithmType::HMAC>(reply_msg.value(), hmac_key.value());
              else
                rpc_status = Status(StatusCode::UNAUTHENTICATED, "Bad request (cannot find hmac key");
            } else {
              reply_packed_msg = MessagePacker::packMessage<MACAlgorithmType::NONE>(reply_msg.value());
            }
            if (!reply_packed_msg.empty()) {
              m_reply.set_message(reply_packed_msg);
              m_reply.set_status(Reply_Status_SUCCESS); // SUCCESS
            } else {
              // TODO : need other reply_status code ( INVALID HMAC )
              m_reply.set_status(Reply_Status_HEADER_INVALID);
            }
          }
        } else {
          // TODO : need other reply_status code ( INVALID HMAC )
          m_reply.set_status(Reply_Status_HEADER_INVALID);
        }
      } else {
        m_reply.set_status(Reply_Status_HEADER_INVALID); // INVALID
      }
    } catch (exception &e) { // INTERNAL
      m_reply.set_status(Reply_Status_INTERNAL);
    } catch (ErrorMsgType err_msg_type) { // KEY EXCHANGE ERROR
      m_reply.set_status((Reply_Status)err_msg_type);
    }
    m_receive_status = RpcCallStatus::FINISH;
    m_responder.Finish(m_reply, rpc_status, this);
  } break;

  default: {
    delete this;
  } break;
  }
}

void MergerService::proceed() {
  switch (m_receive_status) {
  case RpcCallStatus::CREATE: {
    m_receive_status = RpcCallStatus::PROCESS;
    m_service->RequestMergerService(&m_context, &m_request, &m_responder, m_completion_queue, m_completion_queue, this);
  } break;

  case RpcCallStatus::PROCESS: {
    new MergerService(m_service, m_completion_queue, m_routing_table, m_broadcast_check_table, id_mapping_table);
    Status rpc_status;

    try {
      string packed_msg = m_request.message();
      // Forwarding message to other nodes
      if (m_request.broadcast()) {
        string msg_id = m_request.message_id();

        if (m_broadcast_check_table->count(msg_id) > 0) {
          m_reply.set_status(grpc_merger::MsgStatus_Status_DUPLICATED);
          m_reply.set_message("duplicate message");
          m_receive_status = RpcCallStatus::FINISH;
          m_responder.Finish(m_reply, rpc_status, this);
          break;
        }
        uint64_t now = TimeUtil::nowBigInt();
        m_broadcast_check_table->insert({msg_id, now});

        grpc_merger::RequestMsg request;
        request.set_message(packed_msg);
        request.set_broadcast(true);

        ClientContext context;
        std::chrono::time_point deadline = std::chrono::system_clock::now() + GENERAL_SERVICE_TIMEOUT;
        context.set_deadline(deadline);

        grpc_merger::MsgStatus msg_status;

        for (auto &bucket : *m_routing_table) {
          if (!bucket.empty()) {
            auto nodes = bucket.selectRandomAliveNodes(PARALLELISM_ALPHA);
            for (auto &n : nodes) {
              auto stub = GruutMergerService::NewStub(n.getChannelPtr());
              stub->MergerService(&context, request, &msg_status);
            }
          }
        }
      }
      MessageParser msg_parser;
      auto input_data = msg_parser.parseMessage(packed_msg, rpc_status);

      if (rpc_status.ok()) {
        m_reply.set_status(grpc_merger::MsgStatus_Status_SUCCESS); // SUCCESS
        m_reply.set_message("OK");

        MessageHandler msg_handler(&m_context, id_mapping_table);
        msg_handler(input_data.value());
      } else {
        m_reply.set_status(grpc_merger::MsgStatus_Status_INVALID); // INVALID
        m_reply.set_message(rpc_status.error_message());
      }
    } catch (exception &e) { // INTERNAL
      m_reply.set_status(grpc_merger::MsgStatus_Status_INTERNAL);
      m_reply.set_message("Merger internal error");
    }
    m_receive_status = RpcCallStatus::FINISH;
    m_responder.Finish(m_reply, rpc_status, this);

  } break;

  default: {
    delete this;
  } break;
  }
}

void FindNode::proceed() {
  switch (m_receive_status) {
  case RpcCallStatus::CREATE: {
    m_receive_status = RpcCallStatus::PROCESS;
    m_service->RequestFindNode(&m_context, &m_request, &m_responder, m_completion_queue, m_completion_queue, this);
  } break;

  case RpcCallStatus::PROCESS: {
    new FindNode(m_service, m_completion_queue, m_routing_table);

    string target = m_request.target_id();

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

  default: {
    delete this;
  } break;
  }
}

} // namespace net_plugin
} // namespace gruut