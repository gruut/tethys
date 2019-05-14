#pragma once

#include "../proto/include/admin_service.grpc.pb.h"
#include <grpc/support/log.h>
#include <grpcpp/grpcpp.h>

#include "../../../channel_interface/include/channel_interface.hpp"

namespace gruut {
namespace admin_plugin {

using namespace grpc;
using namespace grpc_admin;

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
  void proceed() override {}

private:
  Request req;
  Response res;
  ServerAsyncResponseWriter<Response> responder;
};

} // namespace admin_plugin
} // namespace gruut
