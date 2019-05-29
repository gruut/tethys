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

template <PluginName T1, ControlType T2>
class CommandDelegate {
public:
  CommandDelegate() = default;

  template <PluginName U1 = T1, ControlType U2 = T2, typename = enable_if_t<U1 == PluginName::NET && U2 == ControlType::LOGIN>>
  void operator()(const string &cert, const string &enc_sk_pem, const string &pass) {
    nlohmann::json control_command;
    int control_type = static_cast<int>(ControlType::LOGIN);

    control_command["type"] = control_type;
    control_command["enc_sk"] = enc_sk_pem;
    control_command["cert"] = cert;
    control_command["pass"] = pass;

    app().getChannel<incoming::channels::net_control>().publish(control_command);
  }

  template <PluginName U1 = T1, ControlType U2 = T2, typename = enable_if_t<U1 == PluginName::NET && U2 == ControlType::START>>
  void operator()(ModeType net_mode) {
    nlohmann::json control_command;
    int control_type = static_cast<int>(ControlType::START);
    int net_mode_type = static_cast<int>(net_mode);

    control_command["type"] = control_type;
    control_command["mode"] = net_mode_type;

    app().getChannel<incoming::channels::net_control>().publish(control_command);
  }

  // TODO : send world creation info & local chain info to chain plugin
};

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

unique_ptr<Server> AdminService<ReqSetupKey, ResSetupKey>::initSetup(shared_ptr<SetupService> setup_service, const string &port) {
  string setup_serv_addr("0.0.0.0:");
  setup_serv_addr += port.empty() ? default_setup_port : port;

  ServerBuilder setup_serv_builder;
  setup_serv_builder.AddListeningPort(setup_serv_addr, grpc::InsecureServerCredentials());
  setup_serv_builder.RegisterService(setup_service.get());

  return setup_serv_builder.BuildAndStart();
}

optional<nlohmann::json> AdminService<ReqSetupKey, ResSetupKey>::runSetup(shared_ptr<SetupService> setup_service) {
  auto &promise_user_key = setup_service->getUserKeyPromise();
  auto future = promise_user_key.get_future();

  std::future_status status;
  status = future.wait_for(std::chrono::seconds(30));

  if (status == std::future_status::timeout) {
    logger::ERROR("[SETUP KEY] Timeout occurs when waiting for a user_key_info");
    return {};
  }

  auto sk_cert = future.get();
  if (sk_cert.has_value()) {
    return sk_cert.value();
  }

  return {};
}

optional<string> AdminService<ReqSetupKey, ResSetupKey>::getIdFromCert(const string &cert_pem) {
  try {
    Botan::DataSource_Memory cert_datasource(cert_pem);
    Botan::X509_Certificate cert(cert_datasource);
    auto common_name = cert.subject_info("X520.CommonName");
    return common_name[0];
  } catch (Botan::Exception &exception) {
    logger::ERROR("[SETUP KEY] Wrong Certificate {}", exception.what());
    return {};
  }
}

void AdminService<ReqSetupKey, ResSetupKey>::proceed() {
  switch (receive_status) {
  case AdminRpcCallStatus::CREATE: {
    receive_status = AdminRpcCallStatus::PROCESS;
    service->RequestSetupKey(&context, &req, &responder, completion_queue, completion_queue, this);
    break;
  }
  case AdminRpcCallStatus::PROCESS: {
    new AdminService<ReqSetupKey, ResSetupKey>(service, completion_queue, merger_status, default_setup_port);

    thread([&]() {
      if (merger_status->user_login) {
        string info = "You have been logged in";
        res.set_info(info);

        res.set_success(false);
        receive_status = AdminRpcCallStatus::FINISH;
        responder.Finish(res, Status::OK, this);

        logger::INFO("[SETUP KEY] {}", info);

        return;
      }

      auto &chain = dynamic_cast<ChainPlugin *>(app().getPlugin("ChainPlugin"))->chain();

      auto self_sk = chain.getValueByKey(DataType::SELF_INFO, "self_enc_sk");
      auto self_cert = chain.getValueByKey(DataType::SELF_INFO, "self_cert");

      if (!self_sk.empty() && !self_cert.empty()) {
        string info = "Your key already exist in storage, Please login";
        logger::INFO("[SETUP KEY] {}", info);

        merger_status->user_setup = true;
        res.set_success(false);
        res.set_info(info);
      } else {
        shared_ptr<SetupService> setup_service = make_shared<SetupService>();

        auto setup_port = req.setup_port();
        unique_ptr<Server> setup_server = initSetup(setup_service, setup_port);
        logger::INFO("[SETUP KEY] Waiting for user key info");

        auto user_key_info = runSetup(setup_service);
        if (user_key_info.has_value()) {
          auto enc_sk_pem = json::get<string>(user_key_info.value(), "enc_sk").value_or("");
          auto cert = json::get<string>(user_key_info.value(), "cert").value_or("");

          if (enc_sk_pem.empty() || cert.empty()) {
            string info = "(cert / sk) is empty";
            logger::ERROR("[SETUP KEY] {}", info);

            res.set_success(false);
            res.set_info(info);
          } else {
            auto my_id_b58 = getIdFromCert(cert);
            if (my_id_b58.has_value()) {
              SelfInfo self_info;
              self_info.enc_sk = enc_sk_pem;
              self_info.cert = cert;

              auto my_id = TypeConverter::decodeBase<58>(my_id_b58.value());
              self_info.id = my_id;

              chain.saveSelfInfo(self_info);
              app().setId(my_id);

              merger_status->user_setup = true;
              res.set_success(true);

              logger::INFO("[SETUP KEY] Success");
            } else {
              string info = "Could not find X509 common name field in Certificate";
              res.set_info(info);

              res.set_success(false);
            }
          }
        } else {
          string info = "Could not receive any info from user. please `SETUP KEY` again";
          logger::ERROR("[SETUP KEY] {}", info);

          res.set_info(info);
          res.set_success(false);
        }
        if (setup_server != nullptr)
          setup_server->Shutdown();

        setup_server.reset();
        setup_service.reset();
      }

      receive_status = AdminRpcCallStatus::FINISH;
      responder.Finish(res, Status::OK, this);
    }).detach();

    break;
  }

  default:
    delete this;
    break;
  }
}

bool AdminService<ReqLogin, ResLogin>::checkPassword(const string &enc_sk_pem, const string &pass) {
  try {
    Botan::DataSource_Memory sk_datasource(enc_sk_pem);
    Botan::PKCS8::load_key(sk_datasource, pass);
    return true;
  } catch (Botan::Exception &exception) {
    logger::ERROR("[LOGIN] Error on private key password");
    return false;
  }
}

void AdminService<ReqLogin, ResLogin>::proceed() {
  switch (receive_status) {
  case AdminRpcCallStatus::CREATE: {
    receive_status = AdminRpcCallStatus::PROCESS;
    service->RequestLogin(&context, &req, &responder, completion_queue, completion_queue, this);
    break;
  }
  case AdminRpcCallStatus::PROCESS: {
    new AdminService<ReqLogin, ResLogin>(service, completion_queue, merger_status);
    if (merger_status->user_login) {
      string info = "You have been logged in";
      logger::INFO("[LOGIN] {}", info);
      res.set_info(info);
      res.set_success(false);
    } else {
      auto pass = req.password();
      auto &chain = dynamic_cast<ChainPlugin *>(app().getPlugin("ChainPlugin"))->chain();
      auto self_sk = chain.getValueByKey(DataType::SELF_INFO, "self_enc_sk");
      auto self_cert = chain.getValueByKey(DataType::SELF_INFO, "self_cert");

      if (!self_sk.empty() && !self_cert.empty()) {
        if (checkPassword(self_sk, pass)) {
          CommandDelegate<PluginName::NET, ControlType::LOGIN> forwarder;
          forwarder(self_cert, self_sk, pass);
          merger_status->user_setup = true;
          merger_status->user_login = true;
          res.set_success(true);
          logger::INFO("[LOGIN] Success");
        } else {
          string info = "Fail to login, please check password and try again";
          res.set_info(info);
          res.set_success(false);
          logger::ERROR("[LOGIN] {}", info);
        }
      } else {
        string info = "Could not find key info. Please `Setup Key` again";
        res.set_info(info);
        res.set_success(false);
        logger::ERROR("[LOGIN] {}", info);
      }
    }
    receive_status = AdminRpcCallStatus::FINISH;
    responder.Finish(res, Status::OK, this);
    break;
  }
  default: {
    delete this;
    break;
  }
  }
}

template <>
void AdminService<ReqStart, ResStart>::proceed() {
  switch (receive_status) {
  case AdminRpcCallStatus::CREATE: {
    receive_status = AdminRpcCallStatus::PROCESS;
    service->RequestStart(&context, &req, &responder, completion_queue, completion_queue, this);
    break;
  }
  case AdminRpcCallStatus::PROCESS: {
    new AdminService<ReqStart, ResStart>(service, completion_queue, merger_status);

    auto mode = req.mode();
    if (merger_status->run_mode == static_cast<ModeType>(mode)) {
      string info = "It's already running as a ";
      string mode_type = merger_status->run_mode == ModeType::DEFAULT ? "default mode" : "monitor mode";
      info += mode_type;
      logger::ERROR("[START] {}", info);

      res.set_success(false);
      res.set_info(info);

    } else if (mode == ReqStart_Mode_DEFAULT && !merger_status->user_login) {
      string info = "Could not join a network. Please setup & login OR start as a monitoring mode";
      logger::ERROR("[START] {}", info);

      res.set_success(false);
      res.set_info(info);

    } else {
      auto run_mode = static_cast<ModeType>(mode);
      CommandDelegate<PluginName::NET, ControlType::START> forwarder;
      forwarder(run_mode);

      res.set_success(true);
      merger_status->is_running = true;
      merger_status->run_mode = run_mode;
      app().setRunFlag();

      string mode_type = mode == ReqStart_Mode_DEFAULT ? "default" : "monitor";
      logger::INFO("[START] Success / Mode : {}", mode_type);
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
