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

template <typename T>
class CommandDelegator {
public:
  CommandDelegator() = default;

  void delegate() {
    setControlType();
    nlohmann::json control_command = createControlCommand();

    sendCommand(control_command);
  }

  const int getControlType() const {
    return control_type;
  }

private:
  virtual nlohmann::json createControlCommand() = 0;
  virtual void sendCommand(nlohmann::json control_command) = 0;
  virtual void setControlType() = 0;

protected:
  int control_type;
};

class LoginCommandDelegator : public CommandDelegator<ReqLogin> {
public:
  LoginCommandDelegator(string_view _env_sk_pem, string_view _cert, string_view _pass):
    secret_key(_env_sk_pem), cert(_cert), pass(_pass) {}

private:
  nlohmann::json createControlCommand() override {
    nlohmann::json control_command;

    control_command["type"] = getControlType();
    control_command["enc_sk"] = secret_key;
    control_command["cert"] = cert;
    control_command["pass"] = pass;

    return control_command;
  }

  void sendCommand(nlohmann::json control_command) override {
    app().getChannel<incoming::channels::net_control>().publish(control_command);
  }

  void setControlType() override {
    control_type = static_cast<int>(ControlType::LOGIN);
  }

  string secret_key, cert, pass;
};

class StartCommandDelegator : public CommandDelegator<ReqStart> {
public:
  StartCommandDelegator(RunningMode _net_mode): net_mode(_net_mode) {}

private:
  nlohmann::json createControlCommand() override {
    nlohmann::json control_command;

    control_command["type"] = getControlType();
    control_command["mode"] = net_mode;

    return control_command;
  }

  void sendCommand(nlohmann::json control_command) override {
    app().getChannel<incoming::channels::net_control>().publish(control_command);
  }

  void setControlType() override {
    control_type = static_cast<int>(ControlType::START);
  }

  RunningMode net_mode;
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
    new AdminService<ReqSetupKey, ResSetupKey>(service, completion_queue, default_setup_port);

    thread([&]() {
      if (app().isUserSignedIn()) {
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
        string info = "Your key already exists in storage, Please login";
        logger::INFO("[SETUP KEY] {}", info);

        app().completeUserSetup();

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
              app().completeUserSetup();

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
    new AdminService<ReqLogin, ResLogin>(service, completion_queue);
    if (app().isUserSignedIn()) {
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
          LoginCommandDelegator delegator(self_sk, self_cert, pass);
          delegator.delegate();

          app().completeUserSetup();
          app().completeUserSignedIn();

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
    new AdminService<ReqStart, ResStart>(service, completion_queue);

    if (app().isAppRunning()) {
      string info = "It's already running as a ";
      string mode_type = app().runningMode() == RunningMode::DEFAULT ? "default mode" : "monitor mode";
      info += mode_type;
      logger::ERROR("[START] {}", info);

      res.set_success(false);
      res.set_info(info);

      receive_status = AdminRpcCallStatus::FINISH;
      responder.Finish(res, Status::OK, this);

      break;
    }

    auto mode = req.mode();
    if (mode == ReqStart_Mode_DEFAULT && !app().isUserSignedIn()) {
      string info = "Could not join a network. Please setup & login OR start as a monitoring mode";
      logger::ERROR("[START] {}", info);

      res.set_success(false);
      res.set_info(info);

    } else {
      auto run_mode = static_cast<RunningMode>(mode);
      StartCommandDelegator delegator(run_mode);
      delegator.delegate();

      res.set_success(true);

      app().setRunFlag();
      app().runningMode() = run_mode;

      string mode_type = mode == ReqStart_Mode_DEFAULT ? "default" : "monitor";

      res.set_info("Start to running the node on " + mode_type + " mode");

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
    new AdminService<ReqStatus, ResStatus>(service, completion_queue);

    res.set_alive(app().isAppRunning());

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
