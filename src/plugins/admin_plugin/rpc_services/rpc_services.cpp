#include "include/rpc_services.hpp"
#include "../../../../lib/json/include/json.hpp"
#include "../../../lib/appbase/include/application.hpp"
#include "../../../lib/tethys-utils/src/ecdsa.hpp"
#include "../../../plugins/net_plugin/include/message_parser.hpp"
#include "../../chain_plugin/include/chain_plugin.hpp"
#include "../include/command_delegator.hpp"
#include "../middlewares/include/admin_middleware.hpp"

#include <boost/filesystem.hpp>
#include <exception>
#include <fstream>

// IMPORTANT: This should be located right after `proceed()`
#define BEFORE_PROCEED(SERVICE_CLASS, SERVICE, COMPLETION_QUEUE, ...)                                                                      \
  do {                                                                                                                                     \
    new SERVICE_CLASS(SERVICE, COMPLETION_QUEUE, ##__VA_ARGS__);                                                                           \
    if (call_status == AdminRpcCallStatus::FINISH) {                                                                                       \
      delete this;                                                                                                                         \
      return;                                                                                                                              \
    }                                                                                                                                      \
  } while (0)

#define SET_MIDDLEWARE(SERVICE_CLASS)                                                                                                      \
  auto middleware = make_unique<AdminMiddleware<SERVICE_CLASS>>();                                                                         \
  auto [result, error_message] = middleware->next();                                                                                       \
  if (!result) {                                                                                                                           \
    res.set_info(error_message);                                                                                                           \
    res.set_success(false);                                                                                                                \
                                                                                                                                           \
    sendFinishedMsg(res);                                                                                                                  \
                                                                                                                                           \
    logger::ERROR(error_message);                                                                                                          \
    return;                                                                                                                                \
  }

namespace tethys {
namespace admin_plugin {

namespace fs = boost::filesystem;

using namespace appbase;
using namespace std;

Status SetupService::UserService(ServerContext *context, const Request *request, Reply *response) {
  string err_info;
  MessageParser msg_parser;
  auto input_data = msg_parser.parseMessage(request->message(), err_info);

  if (!input_data.has_value()) {
    user_key_info.set_value({});
    response->set_err_info(err_info);
    response->set_status(Reply_Status_INVALID);
  } else {
    user_key_info.set_value(input_data.value().body);
    response->set_status(Reply_Status_SUCCESS);
  }

  return Status::OK;
}

unique_ptr<Server> SetupKeyService::initSetup(shared_ptr<SetupService> setup_service, const string &port) {
  string setup_serv_addr("0.0.0.0:");
  setup_serv_addr += port.empty() ? default_setup_port : port;

  ServerBuilder setup_serv_builder;
  setup_serv_builder.AddListeningPort(setup_serv_addr, grpc::InsecureServerCredentials());
  setup_serv_builder.RegisterService(setup_service.get());

  return setup_serv_builder.BuildAndStart();
}

optional<nlohmann::json> SetupKeyService::runSetup(shared_ptr<SetupService> setup_service) {
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

optional<string> SetupKeyService::getIdFromCert(const string &cert_pem) {
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

void SetupKeyService::proceed() {
  BEFORE_PROCEED(SetupKeyService, service, completion_queue, default_setup_port);
  SET_MIDDLEWARE(SetupKeyService);

  thread([&]() {
    auto &chain = dynamic_cast<ChainPlugin *>(app().getPlugin("ChainPlugin"))->chain();

    if (isAlreadySetup(chain)) {
      string info = "Your key already exists in storage, Please login";
      logger::INFO("[SETUP KEY] {}", info);

      app().completeUserSetup();

      res.set_success(false);
      res.set_info(info);

      sendFinishedMsg(res);
      return;
    }

    auto user_key_info = waitForUserKeyInfo();
    if (user_key_info.has_value()) {
      auto enc_sk_pem = json::get<string>(user_key_info.value(), "enc_sk").value_or("");
      auto cert = json::get<string>(user_key_info.value(), "cert").value_or("");

      auto [valid, info] = verifyUserKeyInfo(enc_sk_pem, cert);

      if (!valid) {
        res.set_info(info);
        res.set_success(valid);

        sendFinishedMsg(res);

        return;
      }

      SelfInfo self_info;
      self_info.enc_sk = enc_sk_pem;
      self_info.cert = cert;
      chain.saveSelfInfo(self_info);

      auto my_id_b58 = getIdFromCert(cert);
      auto my_id = TypeConverter::decodeBase<58>(my_id_b58.value());
      self_info.id = my_id;
      app().setId(my_id);

      app().completeUserSetup();

      res.set_success(true);

      logger::INFO("[SETUP KEY] Success");
    } else {
      string info = "Could not receive any info from user. please `SETUP KEY` again";
      logger::ERROR("[SETUP KEY] {}", info);

      res.set_info(info);
      res.set_success(false);
    }

    sendFinishedMsg(res);
  }).detach();
} // namespace admin_plugin

bool SetupKeyService::isAlreadySetup(Chain &chain) {
  auto self_sk = chain.getValueByKey(DataType::SELF_INFO, "self_enc_sk");
  if (self_sk.empty())
    return false;

  auto self_cert = chain.getValueByKey(DataType::SELF_INFO, "self_cert");
  if (self_cert.empty())
    return false;

  return true;
}

optional<nlohmann::json> SetupKeyService::waitForUserKeyInfo() {
  shared_ptr<SetupService> setup_service = make_shared<SetupService>();

  auto setup_port = req.setup_port();
  unique_ptr<Server> setup_server = initSetup(setup_service, setup_port);

  logger::INFO("[SETUP KEY] Waiting for user key info");

  auto user_key_info = runSetup(setup_service);

  if (setup_server != nullptr)
    setup_server->Shutdown();

  setup_server.reset();
  setup_service.reset();

  return user_key_info;
}

pair<bool, string> SetupKeyService::verifyUserKeyInfo(string_view enc_sk_pem, const string &cert) {
  string info;

  if (enc_sk_pem.empty() || cert.empty()) {
    info = "(cert / sk) is empty";
    logger::ERROR("[SETUP KEY] {}", info);

    return make_pair(false, info);
  }

  auto my_id_b58 = getIdFromCert(cert);
  if (!my_id_b58.has_value()) {
    info = "Could not find X509 common name field in Certificate";
    logger::ERROR("[SETUP KEY] {}", info);

    return make_pair(false, info);
  }

  return make_pair(true, "");
}

void LoginService::proceed() {
  BEFORE_PROCEED(LoginService, service, completion_queue);
  SET_MIDDLEWARE(LoginService)

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

  sendFinishedMsg(res);
}

bool LoginService::checkPassword(const string &enc_sk_pem, const string &pass) {
  try {
    Botan::DataSource_Memory sk_datasource(enc_sk_pem);
    Botan::PKCS8::load_key(sk_datasource, pass);
    return true;
  } catch (Botan::Exception &exception) {
    logger::ERROR("[LOGIN] Error on private key password");
    return false;
  }
}

void StartService::proceed() {
  BEFORE_PROCEED(StartService, service, completion_queue);

  string info;

  if (app().isAppRunning()) {
    info = "It's already running as a ";
    string mode_type = app().runningMode() == RunningMode::DEFAULT ? "default mode" : "monitor mode";
    info += mode_type;
  }

  if (!app().isWorldLoaded())
    info = "World has not be loaded yet.";
  else if (!app().isChainLoaded())
    info = "Local chain has not be loaded yet.";

  if (!info.empty()) {
    logger::ERROR("[START] {}", info);

    res.set_success(false);
    res.set_info(info);

    sendFinishedMsg(res);

    return;
  }

  auto mode = req.mode();
  if (mode == ReqStart_Mode_DEFAULT && !app().isUserSignedIn()) {
    info = "Could not join a network. Please setup & login OR start as a monitoring mode";
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

  sendFinishedMsg(res);
}

void StatusService::proceed() {
  BEFORE_PROCEED(StatusService, service, completion_queue);

  res.set_alive(app().isAppRunning());

  sendFinishedMsg(res);
}

template <typename TRes>
optional<nlohmann::json> loadJson(const string &path, TRes &return_res) {
  fs::path info_path = path;
  if (info_path.is_relative())
    info_path = fs::current_path() / info_path;

  if (!fs::exists(info_path)) {
    string info = "Cannot find a file. path : " + info_path.string();
    return_res.set_info(info);
    return_res.set_success(false);

    return {};
  }
  try {
    ifstream i(info_path.make_preferred().string());
    nlohmann::json state;
    i >> state;

    return state;

  } catch (nlohmann::json::parse_error &e) {
    string info = "Json parsing err : ";
    info += e.what();
    return_res.set_info(info);
    return_res.set_success(false);
  }

  return {};
}

void LoadWorldService::proceed() {
  BEFORE_PROCEED(LoadWorldService, service, completion_queue);
  SET_MIDDLEWARE(LoadWorldService)

  // TODO : in case of path is `url`, handle it differently

  auto world_state = loadJson(req.path(), res);
  if (!world_state.has_value()) {
    logger::ERROR("[LOAD WORLD] Fail to load world");

    sendFinishedMsg(res);

    return;
  }

  auto &chain = dynamic_cast<ChainPlugin *>(app().getPlugin("ChainPlugin"))->chain();
  try {
    chain.initWorld(world_state.value());

    string info = "Success to load the world";
    res.set_success(true);
    res.set_info(info);

    app().resetLoadChainState();
    app().completeLoadWorld();

    logger::INFO("[LOAD WORLD] " + info);
  } catch (...) {
    string info = "Cannot load world. please check the world json file.";
    res.set_info(info);
    res.set_success(false);

    logger::ERROR("[LOAD WORLD] Fail to load world");
  }

  sendFinishedMsg(res);
}

void LoadChainService::proceed() {
  BEFORE_PROCEED(LoadChainService, service, completion_queue);
  SET_MIDDLEWARE(LoadChainService)

  // TODO : in case of path is `url`, handle it differently

  auto chain_state = loadJson(req.path(), res);
  if (!chain_state.has_value()) {
    logger::ERROR("[LOAD CHAIN] Fail to load local chain");

    sendFinishedMsg(res);

    return;
  }

  auto &chain = dynamic_cast<ChainPlugin *>(app().getPlugin("ChainPlugin"))->chain();
  try {
    auto chain_info = chain.initChain(chain_state.value());
    if (!chain_info.has_value()) {
      string info = "This local chain is in another world. Please check World ID";
      res.set_info(info);
      res.set_success(false);

      logger::ERROR("[LOAD CHAIN] Fail to load local chain");

    } else {
      auto tracker_addresses = chain_info.value();
      LoadChainCommandDelegator delegator(move(tracker_addresses));
      delegator.delegate();

      res.set_success(true);
      logger::INFO("[LOAD CHAIN] Success to load chain");
      app().completeLoadChain();
    }
  } catch (...) {
    string info = "Can not load local chain. please check local chain json file.";
    res.set_info(info);
    res.set_success(false);

    logger::ERROR("[LOAD CHAIN] Fail to load local chain");
  }

  sendFinishedMsg(res);
}

} // namespace admin_plugin
} // namespace tethys