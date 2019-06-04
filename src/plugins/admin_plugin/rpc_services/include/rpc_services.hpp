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
  virtual void proceed() = 0;
};

template <typename Request, typename Response>
class AdminService : public CallService {
public:
  AdminService(GruutAdminService::AsyncService *admin_service, ServerCompletionQueue *cq)
      : service(admin_service), completion_queue(cq), responder(&context) {
    call_status = AdminRpcCallStatus::PROCESS;
  }

  void proceed() override {}

  void sendFinishedMsg(const Response &res) {
    responder.Finish(res, Status::OK, this);

    call_status = AdminRpcCallStatus::FINISH;
  }

protected:
  Request req;
  Response res;
  ServerAsyncResponseWriter<Response> responder;

  GruutAdminService::AsyncService *service;
  ServerCompletionQueue *completion_queue;

  ServerContext context;

  AdminRpcCallStatus call_status{AdminRpcCallStatus::CREATE};
};

class SetupKeyService final : public AdminService<ReqSetupKey, ResSetupKey> {
  using super = AdminService<ReqSetupKey, ResSetupKey>;

public:
  SetupKeyService(GruutAdminService::AsyncService *admin_service, ServerCompletionQueue *cq, string_view setup_port)
      : super(admin_service, cq), default_setup_port(setup_port) {
    service->RequestSetupKey(&context, &req, &responder, completion_queue, completion_queue, this);
  }
  void proceed() override;

private:
  unique_ptr<Server> initSetup(shared_ptr<SetupService> setup_service, const string &port);
  optional<nlohmann::json> runSetup(shared_ptr<SetupService> setup_service);
  optional<string> getIdFromCert(const string &cert_pem);

  string default_setup_port;
};

class LoginService final : public AdminService<ReqLogin, ResLogin> {
  using super = AdminService<ReqLogin, ResLogin>;

public:
  LoginService(GruutAdminService::AsyncService *admin_service, ServerCompletionQueue *cq) : super(admin_service, cq) {
    service->RequestLogin(&context, &req, &responder, completion_queue, completion_queue, this);
  }

  void proceed() override;

private:
  bool checkPassword(const string &enc_sk_pem, const string &pass);
};

class StartService final : public AdminService<ReqStart, ResStart> {
  using super = AdminService<ReqStart, ResStart>;

public:
  StartService(GruutAdminService::AsyncService *admin_service, ServerCompletionQueue *cq) : super(admin_service, cq) {
    service->RequestStart(&context, &req, &responder, completion_queue, completion_queue, this);
  }
  void proceed() override;
};

class StatusService final : public AdminService<ReqStatus, ResStatus> {
  using super = AdminService<ReqStatus, ResStatus>;

public:
  StatusService(GruutAdminService::AsyncService *admin_service, ServerCompletionQueue *cq) : super(admin_service, cq) {
    service->RequestCheckStatus(&context, &req, &responder, completion_queue, completion_queue, this);
  }
  void proceed() override;
};

class LoadWorldService final : public AdminService<ReqLoadWorld, ResLoadWorld> {
  using super = AdminService<ReqLoadWorld, ResLoadWorld>;

public:
  LoadWorldService(GruutAdminService::AsyncService *admin_service, ServerCompletionQueue *cq) : super(admin_service, cq) {
    service->RequestLoadWorld(&context, &req, &responder, completion_queue, completion_queue, this);
  }
  void proceed() override;
};

} // namespace admin_plugin
} // namespace gruut
