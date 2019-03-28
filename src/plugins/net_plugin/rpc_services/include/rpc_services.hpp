#pragma once

#include "../protos/include/general_service.grpc.pb.h"
#include "../protos/include/kademlia_service.grpc.pb.h"
#include <grpc/support/log.h>
#include <grpcpp/grpcpp.h>

#include "../../config/include/network_type.hpp"
#include "../../include/signer_conn_manager.hpp"
#include "../../kademlia/include/node.hpp"
#include "../../kademlia/include/routing.hpp"
#include "../../../channel_interface/include/channel_interface.hpp"

#include <functional>
#include <iostream>
#include <memory>
#include <string>
#include <optional>

using namespace grpc;
using namespace grpc_general;
using namespace kademlia;

namespace gruut {
namespace net_plugin {

enum class RpcCallStatus { CREATE, PROCESS, READ, WAIT, FINISH };

struct NeighborsData {
  std::vector<Node> neighbors;
  uint64_t time_stamp;
  grpc::Status status;
};

struct GeneralData {
  MsgStatus msg_status;
  grpc::Status status;
};

class CallData {
public:
  virtual void proceed() = 0;

protected:
  ServerCompletionQueue *m_completion_queue;
  ServerContext m_context;
  RpcCallStatus m_receive_status;
};

class OpenChannel final : public CallData {
public:
  OpenChannel(GruutGeneralService::AsyncService *service, ServerCompletionQueue *cq, std::shared_ptr<SignerConnTable> signer_table)
      : m_stream(&m_context), m_signer_table(std::move(signer_table)) {

    m_service = service;
    m_completion_queue = cq;
    m_receive_status = RpcCallStatus::CREATE;

    proceed();
  }

private:
  std::string m_signer_id_b64;
  GruutGeneralService::AsyncService *m_service;
  Identity m_request;
  ServerAsyncReaderWriter<ReplyMsg, Identity> m_stream;

  std::shared_ptr<SignerConnTable> m_signer_table;
  void proceed() override;
};

class GeneralService final : public CallData {
public:
  GeneralService(GruutGeneralService::AsyncService *service, ServerCompletionQueue *cq, std::shared_ptr<RoutingTable> routing_table,
                 std::shared_ptr<BroadcastMsgTable> broadcast_check_table)
      : m_responder(&m_context), m_routing_table(std::move(routing_table)), m_broadcast_check_table(std::move(broadcast_check_table)) {

    m_service = service;
    m_completion_queue = cq;
    m_receive_status = RpcCallStatus ::CREATE;

    proceed();
  }

private:
  GruutGeneralService::AsyncService *m_service;
  grpc_general::RequestMsg m_request;
  grpc_general::MsgStatus m_reply;
  ServerAsyncResponseWriter<grpc_general::MsgStatus> m_responder;

  std::shared_ptr<BroadcastMsgTable> m_broadcast_check_table;
  std::shared_ptr<RoutingTable> m_routing_table;
  
  void proceed() override;
  std::optional<InNetMsg> parseMessage(string &packed_msg, Status &return_rpc_status);
  MessageHeader* parseHeader(string &raw_header);
  nlohmann::json getJson(CompressionAlgorithmType comperssion_type, string &raw_body);
  bool validateMsgFormat(MessageHeader *header);
  int convertU8ToU32BE(array<uint8_t, MSG_LENGTH_SIZE> &len_bytes);
};

class FindNode final : public CallData {
public:
  FindNode(KademliaService::AsyncService *service, ServerCompletionQueue *cq, std::shared_ptr<RoutingTable> routing_table)
      : m_responder(&m_context), m_routing_table(routing_table) {

    m_service = service;
    m_completion_queue = cq;
    m_receive_status = RpcCallStatus ::CREATE;

    proceed();
  }

private:
  KademliaService::AsyncService *m_service;
  Target m_request;
  Neighbors m_reply;
  ServerAsyncResponseWriter<Neighbors> m_responder;

  std::shared_ptr<RoutingTable> m_routing_table;
  void proceed() override;
};
} // namespace net_plugin
} // namespace gruut