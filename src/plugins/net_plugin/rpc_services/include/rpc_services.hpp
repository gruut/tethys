#pragma once

#include "../protos/include/kademlia_service.grpc.pb.h"
#include "../protos/include/merger_service.grpc.pb.h"
#include "../protos/include/user_service.grpc.pb.h"
#include <grpc/support/log.h>
#include <grpcpp/grpcpp.h>

#include "../../../channel_interface/include/channel_interface.hpp"
#include "../../config/include/network_type.hpp"
#include "../../include/id_mapping_table.hpp"
#include "../../include/user_conn_table.hpp"
#include "../../include/user_pool_manager.hpp"

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
using namespace grpc_user;
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

class PushService final : public CallData {
public:
  PushService(TethysUserService::AsyncService *service, ServerCompletionQueue *cq, shared_ptr<UserConnTable> user_conn_table,
              shared_ptr<UserPoolManager> user_pool_manager)
      : m_stream(&m_context), m_user_conn_table(move(user_conn_table)), m_user_pool_manager(move(user_pool_manager)) {

    m_service = service;
    m_completion_queue = cq;
    m_receive_status = RpcCallStatus::CREATE;

    proceed();
  }

private:
  string m_user_id_b58;
  TethysUserService::AsyncService *m_service;
  Identity m_request;
  ServerAsyncWriter<Message> m_stream;

  shared_ptr<UserConnTable> m_user_conn_table;
  shared_ptr<UserPoolManager> m_user_pool_manager;
  void proceed() override;
};

class KeyExService final : public CallData {
public:
  KeyExService(TethysUserService::AsyncService *service, ServerCompletionQueue *cq, shared_ptr<UserPoolManager> user_pool_manager)
      : m_responder(&m_context), m_user_pool_manager(move(user_pool_manager)) {
    m_service = service;
    m_completion_queue = cq;
    m_receive_status = RpcCallStatus::CREATE;

    proceed();
  }

private:
  TethysUserService::AsyncService *m_service;
  Request m_request;
  Reply m_reply;
  ServerAsyncResponseWriter<Reply> m_responder;
  shared_ptr<UserPoolManager> m_user_pool_manager;
  void proceed() override;
};

class UserService final : public CallData {
public:
  UserService(TethysUserService::AsyncService *service, ServerCompletionQueue *cq, shared_ptr<UserPoolManager> user_pool_manager,
              shared_ptr<RoutingTable> routing_table)
      : m_responder(&m_context), m_user_pool_manager(move(user_pool_manager)), m_routing_table(move(routing_table)) {
    m_service = service;
    m_completion_queue = cq;
    m_receive_status = RpcCallStatus::CREATE;

    proceed();
  }

private:
  TethysUserService::AsyncService *m_service;
  Request m_request;
  Reply m_reply;
  ServerAsyncResponseWriter<Reply> m_responder;
  shared_ptr<UserPoolManager> m_user_pool_manager;
  shared_ptr<RoutingTable> m_routing_table;
  void proceed() override;
};

class SignerService final : public CallData {
public:
  SignerService(TethysUserService::AsyncService *service, ServerCompletionQueue *cq, shared_ptr<UserPoolManager> user_pool_manager)
      : m_responder(&m_context), m_user_pool_manager(move(user_pool_manager)) {
    m_service = service;
    m_completion_queue = cq;
    m_receive_status = RpcCallStatus::CREATE;

    proceed();
  }

private:
  TethysUserService::AsyncService *m_service;
  Request m_request;
  Reply m_reply;
  ServerAsyncResponseWriter<Reply> m_responder;
  shared_ptr<UserPoolManager> m_user_pool_manager;
  void proceed() override;
};

class MergerService final : public CallData {
public:
  MergerService(GruutMergerService::AsyncService *service, ServerCompletionQueue *cq, shared_ptr<RoutingTable> routing_table,
                shared_ptr<BroadcastMsgTable> broadcast_check_table, shared_ptr<IdMappingTable> id_table)
      : m_responder(&m_context), m_routing_table(move(routing_table)), m_broadcast_check_table(move(broadcast_check_table)),
        id_mapping_table(id_table) {

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
  shared_ptr<IdMappingTable> id_mapping_table;

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