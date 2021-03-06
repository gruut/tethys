#include "include/rpc_services.hpp"
#include "../../../../lib/tethys-utils/src/hmac.hpp"
#include "../../../../lib/tethys-utils/src/time_util.hpp"
#include "../../../../lib/tethys-utils/src/type_converter.hpp"
#include "../../channel_interface/include/channel_interface.hpp"
#include "../include/id_mapping_table.hpp"
#include "../include/message_builder.hpp"
#include "../include/message_packer.hpp"
#include "../include/message_parser.hpp"
#include "../include/user_pool_manager.hpp"
#include "application.hpp"

#include <boost/algorithm/string.hpp>

#include <algorithm>
#include <cctype>
#include <cstring>
#include <exception>
#include <future>
#include <regex>
#include <thread>
#include <type_traits>

namespace tethys {
namespace net_plugin {

using namespace appbase;
using namespace std;

const auto BASE64_REGEX = "^(?:[A-Za-z0-9+/]{4})*(?:[A-Za-z0-9+/]{3}=|[A-Za-z0-9+/]{2}==)?$";
const auto BASE58_REGEX = "^[A-HJ-NP-Za-km-z1-9]*$";
const auto ALPHA_REGEX = "^[A-Z]*$";

class MessageValidator {
public:
  bool validate(nlohmann::json &msg_body) {
    bool check;
    for (auto &[key, val] : msg_body.items()) {
      if (val.is_array())
        continue;
      else if (val.is_null()) {
        if(isNullable(key)) {
          return true;
        } else {
          return false;
        }
      }
      else if (val.is_object()) {
        if (key == "where")
          continue;
        check = validate(val);
        if (!check)
          return false;
      } else {
        check = checkEntry(key, val);
        if (!check)
          return false;
      }
    }
    return true;
  }

private:
  template <typename T>
  bool checkEntry(const string &key, T &val) {
    auto entry_type = getEntryType(key);
    if (val.is_string())
      return validateEntry(entry_type, val);
    else if (val.is_boolean())
      return true;
    return false;
  }

  bool isNullable(const string &key) {
    if (key == "pk") {
      return true;
    }

    return false;
  }

  bool validateEntry(MsgEntryType entry_type, const string &val) {
    switch (entry_type) {
    case MsgEntryType::USER_MODE: {
      return (val == "user" || val == "signer" || val == "all");
    }
    case MsgEntryType::DECIMAL:
    case MsgEntryType::TIMESTAMP: {
      if (!all_of(val.begin(), val.end(), ::isdigit)) {
        logger::ERROR("[MVAL] Error on TIMESTAMP");
        return false;
      }
      return true;
    }
    case MsgEntryType::PK:
    case MsgEntryType::BASE64:
    case MsgEntryType::BASE64_SIG: {
      regex rgx(BASE64_REGEX);
      if (!regex_match(val, rgx)) {
        logger::ERROR("[MVAL] Error on BASE64");
        return false;
      }
      return true;
    }
    case MsgEntryType::BASE64_256: {
      regex rgx(BASE64_REGEX);
      if (val.size() != static_cast<int>(MsgEntryLength::BASE64_256) || !regex_match(val, rgx)) {
        logger::ERROR("[MVAL] Error on BASE64_256");
        return false;
      }
      return true;
    }
    case MsgEntryType::BASE58_256: {
      regex rgx(BASE58_REGEX);
      if (val.size() != static_cast<int>(MsgEntryLength::BASE58_256) || !regex_match(val, rgx)) {
        logger::ERROR("[MVAL] Error on BASE58_256");
        return false;
      }
      return true;
    }
    case MsgEntryType::ALPHA_64: {
      regex rgx(ALPHA_REGEX);
      if (val.size() != static_cast<int>(MsgEntryLength::ALPHA_64) || !regex_match(val, rgx)) {
        logger::ERROR("[MVAL] Error on ALPHA_64");
        return false;
      }
      return true;
    }
    case MsgEntryType::ENC_PRIVATE_PEM: {
      return checkPemContents(MsgEntryType::ENC_PRIVATE_PEM, val);
    }
    case MsgEntryType::PEM: {
      return checkPemContents(MsgEntryType::PEM, val);
    }
    case MsgEntryType::PEM_PK: {
      if (val.find("BEGIN") != string::npos)
        return validateEntry(MsgEntryType::PEM, val);
      else
        return validateEntry(MsgEntryType::PK, val);
    }
    case MsgEntryType::HEX_256: {
      if (val.size() != static_cast<int>(MsgEntryLength::HEX_256) || !all_of(val.begin(), val.end(), ::isxdigit)) {
        logger::ERROR("[MVAL] Error on HEX_256");
        return false;
      }
      return true;
    }
    case MsgEntryType::CONTRACT_ID: {
      vector<string> tokens;
      boost::iter_split(tokens, val, boost::algorithm::first_finder("::"));
      if (tokens.size() != 4)
        return false;
      // TODO : Each tokens may need to be validated
      return true;
    }
    default:
      return false;
    }
  }
  MsgEntryType getEntryType(const string &key) {
    if (key == "time" || key == "btime")
      return MsgEntryType::TIMESTAMP;
    else if (key == "previd" || key == "txid" || key == "receiver" || key == "id" || key == "user" || key == "merger")
      return MsgEntryType::BASE58_256;
    else if (key == "block" || key == "proof" || key == "output")
      return MsgEntryType::BASE64;
    else if (key == "txroot" || key == "usroot" || key == "csroot" || key == "sgroot" || key == "hash" || key == "link" || key == "un")
      return MsgEntryType::BASE64_256;
    else if (key == "sig")
      return MsgEntryType::BASE64_SIG;
    else if (key == "fee" || key == "height" || key == "tx" || key == "signer")
      return MsgEntryType::DECIMAL;
    else if (key == "world" || key == "chain")
      return MsgEntryType::ALPHA_64;
    else if (key == "pk")
      return MsgEntryType::PEM_PK;
    else if (key == "x" || key == "y")
      return MsgEntryType::HEX_256;
    else if (key == "val" || key == "confirm")
      return MsgEntryType::BOOL;
    else if (key == "mode")
      return MsgEntryType::USER_MODE;
    else if (key == "cid")
      return MsgEntryType::CONTRACT_ID;
    else
      return MsgEntryType::NONE;
  }
  bool checkPemContents(MsgEntryType pem_type, const string &pem) {
    string begin_str, end_str;
    switch (pem_type) {
    case MsgEntryType::PEM: {
      begin_str = "-----BEGIN CERTIFICATE-----\n";
      end_str = "\n-----END CERTIFICATE-----";
      break;
    }
    case MsgEntryType::ENC_PRIVATE_PEM: {
      begin_str = "-----BEGIN ENCRYPTED PRIVATE KEY-----\n";
      end_str = "\n-----END ENCRYPTED PRIVATE KEY-----";
      break;
    }
    default: {
      return false;
    }
    }
    auto found1 = pem.find(begin_str);
    auto found2 = pem.find(end_str);
    if (found1 == string::npos || found2 == string::npos) {
      logger::ERROR("[MVAL] Error on PEM");
      return false;
    }
    auto content_len = found2 - begin_str.length();
    auto content = pem.substr(begin_str.length(), content_len);

    string content_without_linebreak;

    vector<string> lines;
    boost::iter_split(lines, content, boost::algorithm::first_finder("\n"));
    for (auto &line : lines)
      content_without_linebreak += line;

    return validateEntry(MsgEntryType::BASE64, content_without_linebreak);
  }
};

class MessageHandler {
public:
  MessageHandler() = default;
  explicit MessageHandler(ServerContext *server_context, shared_ptr<IdMappingTable> table)
      : context(server_context), id_mapping_table(table) {}
  explicit MessageHandler(shared_ptr<UserPoolManager> pool_manager) : user_pool_manager(pool_manager) {}
  optional<OutNetMsg> operator()(InNetMsg &msg) {
    return handle_message(msg);
  }

private:
  optional<OutNetMsg> handle_message(InNetMsg &msg) {
    auto msg_type = msg.type;

    if (msg_type == MessageType::MSG_REQ_BONE || msg_type == MessageType::MSG_REQ_BLOCK) {
      mapping_user_id_to_net_id(msg);
    }

    switch (msg.type) {
    case MessageType::MSG_TX:
      app().getChannel<incoming::channels::transaction>().publish(msg.body);
      return {};
    case MessageType::MSG_BLOCK:
      app().getChannel<incoming::channels::block>().publish(msg.body);
      return {};
    case MessageType::MSG_JOIN:
    case MessageType::MSG_RESPONSE_1:
    case MessageType::MSG_SUCCESS:
      return user_pool_manager->handleMsg(msg);
    case MessageType::MSG_SSIG:
      app().getChannel<incoming::channels::ssig>().publish(msg.body);
      return {};

    case MessageType::MSG_REQ_TX_CHECK:
    case MessageType::MSG_QUERY:
    case MessageType::MSG_REQ_BONE:
    case MessageType::MSG_REQ_BLOCK:
      // TODO : REQ_TX_CHECK , QUERY, REQ_BONE, REQ_BLOCK message must be sent to `chain plugin`
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
  shared_ptr<UserPoolManager> user_pool_manager;
};

void PushService::proceed() {
  switch (m_receive_status) {
  case RpcCallStatus::CREATE: {
    m_receive_status = RpcCallStatus::PROCESS;
    m_context.AsyncNotifyWhenDone(this);
    m_service->RequestPushService(&m_context, &m_request, &m_stream, m_completion_queue, m_completion_queue, this);
  } break;

  case RpcCallStatus::PROCESS: {
    new PushService(m_service, m_completion_queue, m_user_conn_table, m_user_pool_manager);
    m_receive_status = RpcCallStatus::WAIT;
    m_user_id_b58 = TypeConverter::encodeBase<58>(m_request.sender());
    m_user_conn_table->setRpcInfo(m_user_id_b58, &m_stream, this);

    logger::INFO("User/Signer Join. ID : {}", m_user_id_b58);

  } break;

  case RpcCallStatus::WAIT: {
    if (m_context.IsCancelled()) {
      m_user_conn_table->eraseRpcInfo(m_user_id_b58);
      m_user_pool_manager->removeUser(m_user_id_b58);
      m_user_pool_manager->removeTempUser(m_user_id_b58);

      logger::INFO("Disconnected with User/Signer ID: {}", m_user_id_b58);

      delete this;
    }
  } break;

  default:
    break;
  }
}

bool verifyHMAC(string_view packed_msg, vector<uint8_t> &hmac_key) {
  auto hmac_str = packed_msg.substr(packed_msg.length() - 32);
  vector<uint8_t> hmac(hmac_str.begin(), hmac_str.end());
  auto raw_msg = string(packed_msg.substr(0, packed_msg.length() - 32));
  return Hmac::verifyHMAC(raw_msg, hmac, hmac_key);
}

void KeyExService::proceed() {
  switch (m_receive_status) {
  case RpcCallStatus::CREATE: {
    m_receive_status = RpcCallStatus::PROCESS;
    m_service->RequestKeyExService(&m_context, &m_request, &m_responder, m_completion_queue, m_completion_queue, this);
  } break;

  case RpcCallStatus::PROCESS: {
    new KeyExService(m_service, m_completion_queue, m_user_pool_manager);
    string err_info;
    if (app().runningMode() == RunningMode::MONITOR) {
      err_info = "This node is running as a monitoring mode. Please contact other node.";
      m_reply.set_err_info(err_info);
      m_reply.set_status(Reply_Status_INTERNAL);
      m_receive_status = RpcCallStatus::FINISH;
      m_responder.Finish(m_reply, Status::OK, this);
      break;
    }
    try {
      string packed_msg = m_request.message();
      MessageParser msg_parser;
      auto input_data = msg_parser.parseMessage(packed_msg, err_info);
      if (input_data.has_value()) {
        auto msg_type = input_data.value().type;
        auto user_id_b58 = input_data.value().sender_id;

        MessageValidator msg_validator;
        if (!msg_validator.validate(input_data.value().body))
          err_info = "Bad request (message validate fail)";

        if (err_info.empty() && msg_type == MessageType::MSG_SUCCESS) {
          auto hmac_key = m_user_pool_manager->getTempHmacKey(user_id_b58).value_or(vector<uint8_t>());
          if (!verifyHMAC(packed_msg, hmac_key))
            err_info = "Bad request (fail to verify hmac )";
        }
        if (err_info.empty()) {
          MessageHandler msg_handler(m_user_pool_manager);
          auto reply_msg = msg_handler(input_data.value());

          if (reply_msg.has_value()) {
            auto reply_msg_type = reply_msg.value().type;
            string reply_packed_msg;
            if (reply_msg_type == MessageType::MSG_ACCEPT) {
              auto hmac_key = m_user_pool_manager->getHmacKey(user_id_b58);
              if (hmac_key.has_value())
                reply_packed_msg = MessagePacker::packMessage<MACAlgorithmType::HMAC>(reply_msg.value(), hmac_key.value());
              else
                err_info = "Bad request (cannot find hmac key)";

            } else {
              reply_packed_msg = MessagePacker::packMessage<MACAlgorithmType::NONE>(reply_msg.value());
            }
            if (!reply_packed_msg.empty()) {
              m_reply.set_message(reply_packed_msg);
              m_reply.set_status(Reply_Status_SUCCESS);
            }
          }
        }
      }
      if (!err_info.empty()) {
        m_reply.set_err_info(err_info);
        m_reply.set_status(Reply_Status_INVALID);
      }
    } catch (exception &e) {
      m_reply.set_err_info(e.what());
      m_reply.set_status(Reply_Status_INTERNAL);
    } catch (ErrorMsgType err_msg_type) {
      m_reply.set_err_info("ECDH Key exchange error");
      m_reply.set_status((Reply_Status)err_msg_type);
    }
    m_receive_status = RpcCallStatus::FINISH;
    m_responder.Finish(m_reply, Status::OK, this);
  } break;

  default: {
    delete this;
  } break;
  }
}

void UserService::proceed() {
  switch (m_receive_status) {
  case RpcCallStatus::CREATE: {
    m_receive_status = RpcCallStatus::PROCESS;
    m_service->RequestUserService(&m_context, &m_request, &m_responder, m_completion_queue, m_completion_queue, this);
    break;
  }
  case RpcCallStatus::PROCESS: {
    new UserService(m_service, m_completion_queue, m_user_pool_manager, m_routing_table);
    string err_info;
    try {
      string packed_msg = m_request.message();
      MessageParser msg_parser;
      auto input_data = msg_parser.parseMessage(packed_msg, err_info);
      if (input_data.has_value()) {
        auto user_id_b58 = input_data.value().sender_id;
        MessageValidator msg_validator;
        if (!msg_validator.validate(input_data.value().body))
          err_info = "Bad request (message validation fail)";

        if (err_info.empty()) {
          auto hmac_key = m_user_pool_manager->getHmacKey(user_id_b58).value_or(vector<uint8_t>());

          if (verifyHMAC(packed_msg, hmac_key)) {
            MessageHandler msg_handler;
            msg_handler(input_data.value());
            m_reply.set_status(Reply_Status_SUCCESS);

            auto msg_type = input_data.value().type;
            if (msg_type == MessageType::MSG_TX) { // forwarding TX message to other nodes
              grpc_merger::RequestMsg request;

              OutNetMsg out_msg;
              out_msg.type = msg_type;
              out_msg.body = input_data.value().body;
              auto re_packed_msg = MessagePacker::packMessage<MACAlgorithmType::NONE>(out_msg);

              request.set_message(re_packed_msg);
              request.set_broadcast(false);

              ClientContext context;
              std::chrono::time_point deadline = std::chrono::system_clock::now() + GENERAL_SERVICE_TIMEOUT;
              context.set_deadline(deadline);

              grpc_merger::MsgStatus msg_status;

              for (auto &bucket : *m_routing_table) {
                if (!bucket.empty()) {
                  auto nodes = bucket.selectRandomAliveNodes(PARALLELISM_ALPHA);
                  for (auto &n : nodes) {
                    auto stub = TethysMergerService::NewStub(n.getChannelPtr());
                    stub->MergerService(&context, request, &msg_status);
                  }
                }
              }
            }
          } else {
            err_info = "Bad request (fail to verify hmac)";
          }
        }
      }
      if (!err_info.empty()) {
        m_reply.set_err_info(err_info);
        m_reply.set_status(Reply_Status_INVALID);
      }
    } catch (exception &e) {
      m_reply.set_err_info(e.what());
      m_reply.set_status(Reply_Status_INTERNAL);
    }
    m_receive_status = RpcCallStatus::FINISH;
    m_responder.Finish(m_reply, Status::OK, this);
    break;
  }
  default: {
    delete this;
    break;
  }
  }
}

void SignerService::proceed() {
  switch (m_receive_status) {
  case RpcCallStatus::CREATE: {
    m_receive_status = RpcCallStatus::PROCESS;
    m_service->RequestUserService(&m_context, &m_request, &m_responder, m_completion_queue, m_completion_queue, this);
    break;
  }
  case RpcCallStatus::PROCESS: {
    new SignerService(m_service, m_completion_queue, m_user_pool_manager);
    string err_info;
    try {
      string packed_msg = m_request.message();
      MessageParser msg_parser;
      auto input_data = msg_parser.parseMessage(packed_msg, err_info);
      if (input_data.has_value()) {
        auto user_id_b58 = input_data.value().sender_id;
        MessageValidator msg_validator;
        if (!msg_validator.validate(input_data.value().body))
          err_info = "Bad request (message validation fail)";

        if (err_info.empty()) {
          auto hmac_key = m_user_pool_manager->getHmacKey(user_id_b58).value_or(vector<uint8_t>());
          if (verifyHMAC(packed_msg, hmac_key)) {
            MessageHandler msg_handler;
            msg_handler(input_data.value());
            m_reply.set_status(Reply_Status_SUCCESS);
          } else {
            err_info = "Bad request (fail to verify hmac)";
          }
        }
      }
      if (!err_info.empty()) {
        m_reply.set_err_info(err_info);
        m_reply.set_status(Reply_Status_INVALID);
      }
    } catch (exception &e) {
      m_reply.set_err_info(e.what());
      m_reply.set_status(Reply_Status_INTERNAL);
    }
    m_receive_status = RpcCallStatus::FINISH;
    m_responder.Finish(m_reply, Status::OK, this);
    break;
  }
  default: {
    delete this;
    break;
  }
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
    string err_info;
    try {
      string packed_msg = m_request.message();
      MessageParser msg_parser;
      auto input_data = msg_parser.parseMessage(packed_msg, err_info);
      if (input_data.has_value()) {
        MessageValidator msg_validator;
        if (!msg_validator.validate(input_data.value().body))
          err_info = "Bad request (message validate fail)";
      }
      if (err_info.empty()) {
        // Forwarding message to other nodes
        if (m_request.broadcast()) {
          string msg_id = m_request.message_id();

          if (m_broadcast_check_table->count(msg_id) > 0) {
            m_reply.set_status(grpc_merger::MsgStatus_Status_DUPLICATED);
            m_reply.set_message("duplicate message");
            m_receive_status = RpcCallStatus::FINISH;
            m_responder.Finish(m_reply, Status::OK, this);
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
                auto stub = TethysMergerService::NewStub(n.getChannelPtr());
                stub->MergerService(&context, request, &msg_status);
              }
            }
          }
        }
        if (app().runningMode() != RunningMode::MONITOR || input_data.value().type != MessageType::MSG_TX) {
          MessageHandler msg_handler(&m_context, id_mapping_table);
          msg_handler(input_data.value());
        }
        m_reply.set_status(grpc_merger::MsgStatus_Status_SUCCESS);
      } else {
        m_reply.set_status(grpc_merger::MsgStatus_Status_INVALID);
        m_reply.set_message(err_info);
      }
    } catch (exception &e) {
      m_reply.set_status(grpc_merger::MsgStatus_Status_INTERNAL);
      m_reply.set_message(e.what());
    }
    m_receive_status = RpcCallStatus::FINISH;
    m_responder.Finish(m_reply, Status::OK, this);

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

    string sender_id = m_request.sender_id();
    string sender_ip_addr = m_request.sender_address();
    string sender_port = m_request.sender_port();

    Node node(Hash<160>::sha1(sender_id), sender_id, sender_ip_addr, sender_port);
    m_routing_table->addPeer(move(node));

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
} // namespace tethys