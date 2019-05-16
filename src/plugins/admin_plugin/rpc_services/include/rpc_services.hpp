#pragma once

#include "../../../net_plugin/rpc_services/protos/include/user_service.grpc.pb.h"
#include "../proto/include/admin_service.grpc.pb.h"
#include <atomic>
#include <future>
#include <grpc/support/log.h>
#include <grpcpp/grpcpp.h>

#include "../../../channel_interface/include/channel_interface.hpp"

namespace gruut {
namespace admin_plugin {

using namespace grpc;
using namespace grpc_user;
using namespace grpc_admin;

struct MergerStatus {
  atomic<bool> user_setup{false};
  atomic<bool> is_running{false};
};

class SetupService final : public GruutUserService::Service {
public:
  SetupService() = default;
  Status UserService(ServerContext *context, const Request *request, Reply *response) override;
  promise<optional<nlohmann::json>> &getUserKeyPromise() {
    return user_key_info;
  }

private:
  promise<optional<nlohmann::json>> user_key_info;
};

enum class AdminRpcCallStatus { CREATE, PROCESS, FINISH };

class CallService {
public:
  CallService(GruutAdminService::AsyncService *admin_service, ServerCompletionQueue *cq, shared_ptr<MergerStatus> m_status)
      : merger_status(m_status) {
    service = admin_service;
    completion_queue = cq;
  }

  virtual void proceed() = 0;

protected:
  shared_ptr<MergerStatus> merger_status;
  GruutAdminService::AsyncService *service;
  ServerCompletionQueue *completion_queue;
  ServerContext context;
  AdminRpcCallStatus receive_status{AdminRpcCallStatus::CREATE};
};

template <typename Request, typename Response>
class AdminService final : public CallService {
public:
  AdminService(GruutAdminService::AsyncService *admin_service, ServerCompletionQueue *cq, shared_ptr<MergerStatus> m_status)
      : CallService(admin_service, cq, m_status), responder(&context) {
    proceed();
  }
  void proceed() override;

private:
  Request req;
  Response res;
  ServerAsyncResponseWriter<Response> responder;
};

template <>
class AdminService<ReqSetup, ResSetup> final : public CallService {
public:
  AdminService(GruutAdminService::AsyncService *admin_service, ServerCompletionQueue *cq, shared_ptr<MergerStatus> m_status,
               const string &setup_port)
      : CallService(admin_service, cq, m_status), responder(&context), port(setup_port) {
    proceed();
  }
  void proceed() override;

private:
  string port;
  ReqSetup req;
  ResSetup res;
  ServerAsyncResponseWriter<ResSetup> responder;
  optional<nlohmann::json> runSetup(shared_ptr<SetupService> setup_service);
  unique_ptr<Server> initSetup(shared_ptr<SetupService> setup_service);
};

} // namespace admin_plugin
} // namespace gruut
