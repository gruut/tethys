#include "include/rpc_services.hpp"
#include "../../../../lib/json/include/json.hpp"
#include "../../../lib/appbase/include/application.hpp"
#include "../../../lib/gruut-utils/src/ecdsa.hpp"
#include "../../../plugins/net_plugin/include/message_parser.hpp"
#include <exception>

namespace gruut {
namespace admin_plugin {

using namespace appbase;
using namespace std;

Status SetupService::UserService(ServerContext *context, const Request *request, Reply *response) {
  Status rpc_status;
  MessageParser msg_parser;
  auto input_data = msg_parser.parseMessage(request->message(), rpc_status);
  if (!input_data.has_value()) {
    user_key_info.set_value({});
    response->set_status(Reply_Status_INVALID);
    return rpc_status;
  }
  user_key_info.set_value(input_data.value().body);
  response->set_status(Reply_Status_SUCCESS);
  return Status::OK;
}

optional<nlohmann::json> AdminService<ReqSetup, ResSetup>::runSetup() {
  string setup_serv_addr("0.0.0.0:" + port);
  SetupService setup_service;

  ServerBuilder setup_serv_builder;
  setup_serv_builder.AddListeningPort(setup_serv_addr, grpc::InsecureServerCredentials());
  setup_serv_builder.RegisterService(&setup_service);

  unique_ptr<Server> setup_server(setup_serv_builder.BuildAndStart());

  auto &promise_user_key = setup_service.getUserKeyPromise();
  auto user_key_info = promise_user_key.get_future();

  user_key_info.wait(); // waiting for user key info

  if (setup_server != nullptr)
    setup_server->Shutdown();
  setup_server.reset();

  if (user_key_info.get().has_value()) {
    return user_key_info.get().value();
  }
  return {};
}

// TODO : handle admin request
void AdminService<ReqSetup, ResSetup>::proceed() {
  switch (receive_status) {
  case AdminRpcCallStatus::CREATE: {
    receive_status = AdminRpcCallStatus::PROCESS;
    service->RequestSetup(&context, &req, &responder, completion_queue, completion_queue, this);
    break;
  }
  case AdminRpcCallStatus::PROCESS: {
    new AdminService<ReqSetup, ResSetup>(service, completion_queue, merger_status, port);
    auto pass = req.password();
    bool has_key = false;

    // TODO :  get encrypted sk & cert from `Storage`

    if (!has_key) { // no key info in storage
      auto user_key_info = runSetup();

      if (!user_key_info.has_value()) {
        auto enc_sk_pem = json::get<string>(user_key_info.value(), "enc_sk");
        auto cert = json::get<string>(user_key_info.value(), "cert");
        if (!enc_sk_pem.has_value() || !cert.has_value())
          res.set_success(false);
        else {
          // TODO : 1. get decrypted sk using password
          //        2. send  sk & cert to `Storage`
          merger_status->user_setup = true;
          res.set_success(true);
        }
      }
    }
    receive_status = AdminRpcCallStatus::FINISH;
    responder.Finish(res, Status::OK, this);
    break;
  }
  default:
    delete this;
    break;
  }
}

template <>
void AdminService<ReqStart, ResStart>::proceed() {}

template <>
void AdminService<ReqStop, ResStop>::proceed() {}

template <>
void AdminService<ReqStatus, ResStatus>::proceed() {
  switch (receive_status) {
  case AdminRpcCallStatus::CREATE: {
    receive_status = AdminRpcCallStatus::PROCESS;
    service->RequestCheckStatus(&context, &req, &responder, completion_queue, completion_queue, this);
  } break;

  case AdminRpcCallStatus::PROCESS: {
    new AdminService<ReqStatus, ResStatus>(service, completion_queue, merger_status);

    res.set_alive(merger_status->is_running);

    receive_status = AdminRpcCallStatus::FINISH;
    responder.Finish(res, Status::OK, this);
  } break;

  default: {
    delete this;
  } break;
  }
}

} // namespace admin_plugin
} // namespace gruut
