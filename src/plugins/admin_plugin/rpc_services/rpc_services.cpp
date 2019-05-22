#include "include/rpc_services.hpp"
#include "../../../../lib/json/include/json.hpp"
#include "../../../lib/appbase/include/application.hpp"
#include "../../../lib/gruut-utils/src/ecdsa.hpp"
#include "../../../plugins/net_plugin/include/message_parser.hpp"
#include "../../chain_plugin/include/chain_plugin.hpp"
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

unique_ptr<Server> AdminService<ReqSetup, ResSetup>::initSetup(shared_ptr<SetupService> setup_service) {
  string setup_serv_addr("0.0.0.0:" + port);

  ServerBuilder setup_serv_builder;
  setup_serv_builder.AddListeningPort(setup_serv_addr, grpc::InsecureServerCredentials());
  setup_serv_builder.RegisterService(setup_service.get());

  return setup_serv_builder.BuildAndStart();
}

optional<nlohmann::json> AdminService<ReqSetup, ResSetup>::runSetup(shared_ptr<SetupService> setup_service) {
  auto &promise_user_key = setup_service->getUserKeyPromise();
  auto user_key_info = promise_user_key.get_future();

  user_key_info.wait(); // waiting for user key info

  if (user_key_info.get().has_value()) {
    return user_key_info.get().value();
  }
  return {};
}

bool AdminService<ReqSetup, ResSetup>::checkPassword(const string &enc_sk_pem, const string &pass) {
  try {
    Botan::DataSource_Memory sk_datasource(enc_sk_pem);
    Botan::PKCS8::load_key(sk_datasource, pass);
    return true;
  } catch (Botan::Exception &exception) {
    logger::ERROR("[SETUP] Error on private key password");
    return false;
  }
}

void AdminService<ReqSetup, ResSetup>::sendKeyInfoToNet(const string &cert, const string &enc_sk_pem, const string &pass) {
  nlohmann::json control_command;
  int control_type = static_cast<int>(NetControlType::SETUP);

  control_command["type"] = control_type;
  control_command["enc_sk"] = enc_sk_pem;
  control_command["cert"] = cert;
  control_command["pass"] = pass;

  app().getChannel<incoming::channels::net_control>().publish(control_command);
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
    if (merger_status->user_setup) {
      res.set_success(true);
      receive_status = AdminRpcCallStatus::FINISH;
      responder.Finish(res, Status::OK, this);
      logger::INFO("It's already setup");
      break;
    }
    auto pass = req.password();
    auto &chain = dynamic_cast<ChainPlugin *>(app().getPlugin("ChainPlugin"))->chain();
    auto self_sk = chain.getValueByKey(DataType::SELF_INFO, "self_enc_sk");
    auto self_cert = chain.getValueByKey(DataType::SELF_INFO, "self_cert");

    if (!self_sk.empty() && !self_cert.empty()) {
      if (checkPassword(self_sk, pass)) {
        merger_status->user_setup = true;
        sendKeyInfoToNet(self_cert, self_sk, pass);
        res.set_success(true);
      }
    } else { // no key info in storage
      shared_ptr<SetupService> setup_service = make_shared<SetupService>();
      unique_ptr<Server> setup_server = initSetup(setup_service);
      auto user_key_info = runSetup(setup_service);

      if (user_key_info.has_value()) {
        auto enc_sk_pem = json::get<string>(user_key_info.value(), "enc_sk");
        auto cert = json::get<string>(user_key_info.value(), "cert");
        if (!enc_sk_pem.has_value() || !cert.has_value())
          res.set_success(false);
        else if (enc_sk_pem.value().empty() || cert.value().empty())
          res.set_success(false);
        else if (!checkPassword(enc_sk_pem.value(), pass))
          res.set_success(false);
        else {
          SelfInfo self_info;
          self_info.enc_sk = enc_sk_pem.value();
          self_info.cert = cert.value();
          chain.saveSelfInfo(self_info);
          sendKeyInfoToNet(cert.value(), enc_sk_pem.value(), pass);
          merger_status->user_setup = true;
          res.set_success(true);
        }
      }
      if (setup_server != nullptr)
        setup_server->Shutdown();
      setup_server.reset();
      setup_service.reset();
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
