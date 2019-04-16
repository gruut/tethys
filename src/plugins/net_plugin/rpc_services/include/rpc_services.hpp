#pragma once

#include "../protos/include/kademlia_service.grpc.pb.h"
#include "../protos/include/merger_service.grpc.pb.h"
#include "../protos/include/signer_service.grpc.pb.h"
#include <grpc/support/log.h>
#include <grpcpp/grpcpp.h>

#include "../../../channel_interface/include/channel_interface.hpp"
#include "../../config/include/network_type.hpp"
#include "../../include/signer_conn_table.hpp"
#include "../../include/signer_pool_manager.hpp"
#include "../../kademlia/include/node.hpp"
#include "../../kademlia/include/routing.hpp"

#include <functional>
#include <iostream>
#include <memory>
#include <optional>
#include <string>

namespace gruut {
namespace net_plugin {

using namespace grpc;
using namespace grpc_merger;
using namespace grpc_signer;
using namespace kademlia;

enum class RpcCallStatus { CREATE, PROCESS, READ, WAIT, FINISH };

struct NeighborsData {
  vector<Node> neighbors;
  uint64_t time_stamp;
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

class OpenChannelWithSigner final : public CallData {
public:
  OpenChannelWithSigner(GruutSignerService::AsyncService *service, ServerCompletionQueue *cq, shared_ptr<SignerConnTable> signer_conn_table,
                        shared_ptr<SignerPoolManager> signer_pool_manager)
      : m_stream(&m_context), m_signer_conn_table(move(signer_conn_table)), m_signer_pool_manager(move(signer_pool_manager)) {

    m_service = service;
    m_completion_queue = cq;
    m_receive_status = RpcCallStatus::CREATE;

    proceed();
  }

private:
  string m_signer_id_b58;
  GruutSignerService::AsyncService *m_service;
  Identity m_request;
  ServerAsyncReaderWriter<Request, Identity> m_stream;

  shared_ptr<SignerConnTable> m_signer_conn_table;
  shared_ptr<SignerPoolManager> m_signer_pool_manager;
  void proceed() override;
};

class SignerService final : public CallData {
public:
  SignerService(GruutSignerService::AsyncService *service, ServerCompletionQueue *cq, shared_ptr<SignerPoolManager> signer_pool_manager)
      : m_responder(&m_context), m_signer_pool_manager(move(signer_pool_manager)) {
    m_service = service;
    m_completion_queue = cq;
    m_receive_status = RpcCallStatus::CREATE;
  }

private:
  GruutSignerService::AsyncService *m_service;
  Request m_request;
  Reply m_reply;
  ServerAsyncResponseWriter<Reply> m_responder;

  shared_ptr<SignerPoolManager> m_signer_pool_manager;
  grpc::Status verifyHMAC(string_view packed_msg, vector<uint8_t> &hmac_key);
  void proceed() override;
};

class MergerService final : public CallData {
public:
  MergerService(GruutMergerService::AsyncService *service, ServerCompletionQueue *cq, shared_ptr<RoutingTable> routing_table,
                shared_ptr<BroadcastMsgTable> broadcast_check_table)
      : m_responder(&m_context), m_routing_table(move(routing_table)), m_broadcast_check_table(move(broadcast_check_table)) {

    m_service = service;
    m_completion_queue = cq;
    m_receive_status = RpcCallStatus ::CREATE;

    proceed();
  }

private:
  GruutMergerService::AsyncService *m_service;
  grpc_merger::RequestMsg m_request;
  grpc_merger::MsgStatus m_reply;
  ServerAsyncResponseWriter<grpc_merger::MsgStatus> m_responder;

  shared_ptr<BroadcastMsgTable> m_broadcast_check_table;
  shared_ptr<RoutingTable> m_routing_table;

  void proceed() override;
};

class FindNode final : public CallData {
public:
  FindNode(KademliaService::AsyncService *service, ServerCompletionQueue *cq, shared_ptr<RoutingTable> routing_table)
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

  shared_ptr<RoutingTable> m_routing_table;
  void proceed() override;
};
} // namespace net_plugin
} // namespace gruut