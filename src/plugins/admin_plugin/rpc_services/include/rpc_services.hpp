#pragma once

#include "../../../net_plugin/rpc_services/protos/include/user_service.grpc.pb.h"
#include "../../include/admin_type.hpp"
#include "../proto/include/admin_service.grpc.pb.h"
#include <atomic>
#include <future>
#include <grpc/support/log.h>
#include <grpcpp/grpcpp.h>
#include <optional>

#include "../../../channel_interface/include/channel_interface.hpp"

namespace gruut {
namespace admin_plugin {

using namespace grpc;
using namespace grpc_user;
using namespace grpc_admin;

class SetupService final : public TethysUserService::Service {
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
  CallService(GruutAdminService::AsyncService *admin_service, ServerCompletionQueue *cq) {
    service = admin_service;
    completion_queue = cq;
  }

  virtual void proceed() = 0;

protected:
  GruutAdminService::AsyncService *service;
  ServerCompletionQueue *completion_queue;
  ServerContext context;
  AdminRpcCallStatus receive_status{AdminRpcCallStatus::CREATE};
};

template <typename Request, typename Response>
class AdminService final : public CallService {
public:
  AdminService(GruutAdminService::AsyncService *admin_service, ServerCompletionQueue *cq)
      : CallService(admin_service, cq), responder(&context) {
    proceed();
  }
  void proceed() override;

private:
  Request req;
  Response res;
  ServerAsyncResponseWriter<Response> responder;
};

template <>
class AdminService<ReqSetupKey, ResSetupKey> final : public CallService {
public:
  AdminService(GruutAdminService::AsyncService *admin_service, ServerCompletionQueue *cq,
               const string &setup_port)
      : CallService(admin_service, cq), responder(&context), default_setup_port(setup_port) {
    proceed();
  }
  void proceed() override;

private:
  string default_setup_port;
  ReqSetupKey req;
  ResSetupKey res;
  ServerAsyncResponseWriter<ResSetupKey> responder;
  unique_ptr<Server> initSetup(shared_ptr<SetupService> setup_service, const string &port);
  optional<nlohmann::json> runSetup(shared_ptr<SetupService> setup_service);
  optional<string> getIdFromCert(const string &cert_pem);
};

template <>
class AdminService<ReqLogin, ResLogin> final : public CallService {
public:
  AdminService(GruutAdminService::AsyncService *admin_service, ServerCompletionQueue *cq)
      : CallService(admin_service, cq), responder(&context) {
    proceed();
  }
  void proceed() override;

private:
  ReqLogin req;
  ResLogin res;
  ServerAsyncResponseWriter<ResLogin> responder;
  bool checkPassword(const string &enc_sk_pem, const string &pass);
};

} // namespace admin_plugin
} // namespace gruut
