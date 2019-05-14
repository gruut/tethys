#include "include/admin_plugin.hpp"
#include "../../lib/json/include/json.hpp"
#include "rpc_services/include/rpc_services.hpp"

#include <atomic>
#include <boost/asio/steady_timer.hpp>

namespace gruut {
using namespace std;
using namespace admin_plugin;

const auto ADMIN_REQ_CHECK_PERIOD = std::chrono::milliseconds(100);

class AdminPluginImpl {
public:
  GruutAdminService::AsyncService admin_service;

  unique_ptr<Server> admin_server;
  unique_ptr<ServerCompletionQueue> completion_queue;

  string admin_port;
  string admin_ip;

  unique_ptr<boost::asio::steady_timer> admin_req_check_timer;

  ~AdminPluginImpl() {
    if (admin_server != nullptr)
      admin_server->Shutdown();
    if (completion_queue != nullptr)
      completion_queue->Shutdown();
  }

  void initialize() {
    initializeAdminServer();
    registerService();

    admin_req_check_timer = make_unique<boost::asio::steady_timer>(app().getIoContext());
  }

  void registerService() {
    new AdminService<ReqSetup, ResSetup>(&admin_service, completion_queue.get());
    new AdminService<ReqStart, ResStart>(&admin_service, completion_queue.get());
    new AdminService<ReqStop, ResStop>(&admin_service, completion_queue.get());
    new AdminService<ReqStatus, ResStatus>(&admin_service, completion_queue.get());
  }

  void start() {
    monitorCompletionQueue();
  }

  void initializeAdminServer() {
    ServerBuilder builder;
    // TODO : secure channel may be used.
    builder.AddListeningPort(admin_ip + ":" + admin_port, grpc::InsecureServerCredentials());
    builder.RegisterService(&admin_service);

    completion_queue = builder.AddCompletionQueue();
    admin_server = builder.BuildAndStart();
  }

  void monitorCompletionQueue() {
    admin_req_check_timer->expires_from_now(ADMIN_REQ_CHECK_PERIOD);
    admin_req_check_timer->async_wait([this](boost::system::error_code ec) {
      if (!ec) {
        void *tag;
        bool ok;

        gpr_timespec deadline;
        deadline.clock_type = GPR_TIMESPAN;
        deadline.tv_sec = 0;
        deadline.tv_nsec = 10000000; // 10 milliseconds

        auto queue_state = completion_queue->AsyncNext(&tag, &ok, deadline);
        if (ok && queue_state == CompletionQueue::NextStatus::GOT_EVENT) {
          static_cast<CallService *>(tag)->proceed();
        }
      } else {
        logger::ERROR("Error from admin_req_check_timer: {}", ec.message());
      }

      monitorCompletionQueue();
    });
  }
};

AdminPlugin::AdminPlugin() : impl(new AdminPluginImpl()) {}

void AdminPlugin::setProgramOptions(po::options_description &cfg) {
  cfg.add_options()("admin-port", po::value<string>()->composing());
  cfg.add_options()("admin-ip", po::value<string>()->composing());
}

void AdminPlugin::pluginInitialize(const variables_map &options) {
  logger::INFO("AdminPlugin Initialize");

  if (options.count("admin-port")) {
    auto port = options["admin-port"].as<string>();

    impl->admin_port = port;
  }

  if (options.count("admin-ip")) {
    auto ip = options["admin-ip"].as<string>();

    impl->admin_ip = ip;
  }

  // TODO : need to define internal channel to communicate with other plugins

  impl->initialize();
}

void AdminPlugin::pluginStart() {
  logger::INFO("AdminPlugin Start");

  impl->start();
}

AdminPlugin::~AdminPlugin() {
  impl.reset();
}

} // namespace gruut
