#include "include/admin_plugin.hpp"
#include "../../lib/json/include/json.hpp"
#include "rpc_services/include/rpc_services.hpp"

#include <boost/asio/steady_timer.hpp>

namespace tethys {
using namespace std;
using namespace admin_plugin;

const auto ADMIN_REQ_CHECK_PERIOD = std::chrono::milliseconds(100);

class AdminPluginImpl {
public:
  TethysAdminService::AsyncService admin_service;

  unique_ptr<Server> admin_server;
  unique_ptr<ServerCompletionQueue> completion_queue;

  string admin_address;
  string setup_port;

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

    monitorCompletionQueue();
  }

  void registerService() {
    new SetupKeyService(&admin_service, completion_queue.get(), setup_port);
    new LoginService(&admin_service, completion_queue.get());
    new StartService(&admin_service, completion_queue.get());
    new StatusService(&admin_service, completion_queue.get());
  }

  void start() {}

  void initializeAdminServer() {
    ServerBuilder builder;
    // TODO : secure channel may be used.
    builder.AddListeningPort(admin_address, grpc::InsecureServerCredentials());
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
          static_cast <CallService *>(tag)->proceed();
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
  cfg.add_options()("admin-address", po::value<string>()->composing());
  cfg.add_options()("setup-port", po::value<string>()->composing());
}

void AdminPlugin::pluginInitialize(const variables_map &options) {
  logger::INFO("AdminPlugin Initialize");

  if (options.count("admin-address")) {
    auto admin_address = options["admin-address"].as<string>();

    impl->admin_address = admin_address;
  }
  if (options.count("setup-port")) {
    auto setup_port = options["setup-port"].as<string>();

    impl->setup_port = setup_port;
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

} // namespace tethys
