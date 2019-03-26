#include "include/rpc_services.hpp"

#include "../../../../lib/gruut-utils/src/time_util.hpp"
#include "../../channel_interface/include/channel_interface.hpp"
#include "../include/msg_handler.hpp"
#include "application.hpp"

#include <cstring>
#include <exception>
#include <future>
#include <thread>

namespace gruut {
namespace net_plugin {

using namespace appbase;

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

      // TODO : Make internal message and then send it to core
      m_signer_table->eraseRpcInfo(m_signer_id_b64);
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

        if (m_broadcast_check_table->count(msg_id) > 0){
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

      MessageHandler msg_handler;
      auto input_data = msg_handler.unpackMsg(packed_msg, rpc_status);

      auto &in_msg_channel = app().getChannel<incoming::channels::network::channel_type>();
      in_msg_channel.publish(input_data);

      if (rpc_status.ok()) {
        m_reply.set_status(grpc_general::MsgStatus_Status_SUCCESS); // SUCCESS
        m_reply.set_message("OK");
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

    // TODO: gruut util의  Time객체 이용할 것.
    uint64_t now = static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count());
    m_reply.set_time_stamp(now);

    Status rpc_status = Status::OK;
    m_receive_status = RpcCallStatus::FINISH;
    m_responder.Finish(m_reply, Status::OK, this);
  } break;

  default: { delete this; } break;
  }
}

} // namespace net_plugin
} // namespace gruut