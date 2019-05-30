#include <boost/asio/signal_set.hpp>
#include <boost/filesystem.hpp>
#include <iostream>

#include <application.hpp>

#include "include/application.hpp"

namespace appbase {
namespace fs = boost::filesystem;

class ProgramOptions {
public:
  ProgramOptions() : options_description("Options") {}

  po::options_description options_description;
  po::options_description config_options;

  po::variables_map options_map;
};

Application::Application() : program_options(make_unique<ProgramOptions>()), io_context_ptr(make_shared<boost::asio::io_context>()) {}

Application::~Application() = default;

Application &Application::instance() {
  static Application app;
  return app;
}

bool Application::initializeImpl() {
  initializePlugins();

  return true;
}

void Application::setProgramOptions(po::options_description &plugin_cfg_opts) {
  po::options_description app_cfg_opts("Application Config Options");
  po::options_description app_cli_opts("Application Command Line Options");

  app_cfg_opts.add_options()("plugin", po::value<vector<string>>()->composing(), "You can specify multiple times.");

  app_cli_opts.add_options()("config-path,c", po::value<string>()->default_value("config.ini"),
                             "Configuration file path (relative/absolute file path)")("help,h", "Print help message");

  program_options->config_options.add(app_cfg_opts);
  program_options->config_options.add(plugin_cfg_opts);

  program_options->options_description.add(app_cfg_opts);
  program_options->options_description.add(app_cli_opts);
}

bool Application::parseProgramOptions(int argc, char **argv) {
  po::store(po::parse_command_line(argc, argv, program_options->options_description), program_options->options_map);

  if (program_options->options_map.count("help")) {
    cout << program_options->options_description << endl;
    return false;
  }

  fs::path config_path;
  if (program_options->options_map.count("config-path")) {
    config_path = program_options->options_map["config-path"].as<string>();

    if (config_path.is_relative())
      config_path = fs::current_path() / config_path;

    if (!fs::exists(config_path)) {
      logger::ERROR("Configuration file does not exist: {}", config_path.string());
      return false;
    }
  } else {
    logger::ERROR("Read the default configuration file: {}", config_path.string());
    config_path = fs::current_path() / "config.ini";
  }
  po::store(po::parse_config_file<char>(config_path.make_preferred().string().c_str(), program_options->config_options),
            program_options->options_map);

  return true;
}

void Application::initializePlugins() {
  auto plugin_names = program_options->options_map.at("plugin").as<vector<string>>();

  auto result = find(plugin_names.begin(), plugin_names.end(), "AdminPlugin");
  if (result == plugin_names.end()) {
    logger::ERROR("'AdminPlugin' is necessarily required. Please make sure that 'AdminPlugin' is configured in the configuration file.");

    exit(1);
  }
  for_each(begin(plugin_names), end(plugin_names), [this](const string &plugin_name) {
    auto it = app_plugins_map.find(plugin_name);
    if (it != app_plugins_map.end()) {
      if (it->second->getState() == AbstractPlugin::plugin_state::registered) {
        it->second->initialize(program_options->options_map);
      }
    }
  });
}

void Application::start() {
  thread proactor([this](){io_context_ptr->run();});

  startInitializedPlugins();

  registerErrorSignalHandlers();

  proactor.join();

  shutdown();
}

void Application::startInitializedPlugins() {
  while(!app().isAppRunning());

  for (auto &[_, plugin_ptr] : app_plugins_map) {
    if(plugin_ptr->getState() == AbstractPlugin::plugin_state::initialized) {
      plugin_ptr->start();
    }
  }
}

void Application::registerErrorSignalHandlers() {
  shared_ptr<boost::asio::signal_set> sigint_set(new boost::asio::signal_set(*io_context_ptr, SIGINT));
  sigint_set->async_wait([sigint_set, this](const boost::system::error_code &err, int num) {
      logger::ERROR("SIGINT Received: {}", err.message());
      sigint_set->cancel();
      quit();
  });

  shared_ptr<boost::asio::signal_set> sigterm_set(new boost::asio::signal_set(*io_context_ptr, SIGTERM));
  sigterm_set->async_wait([sigterm_set, this](const boost::system::error_code &err, int num) {
      logger::ERROR("SIGTERM Received: {}", err.message());
      sigterm_set->cancel();
      quit();
  });

  shared_ptr<boost::asio::signal_set> sigpipe_set(new boost::asio::signal_set(*io_context_ptr, SIGPIPE));
  sigpipe_set->async_wait([sigpipe_set, this](const boost::system::error_code &err, int num) {
      logger::ERROR("SIGPIPE Received: {}", err.message());
      sigpipe_set->cancel();
      quit();
  });
}

void Application::quit() {
  io_context_ptr->stop();
}

void Application::shutdown() {
  logger::INFO("Application Shutdown");

  app_plugins_map.clear();
  io_context_ptr->reset();
}

AbstractPlugin* Application::getPlugin(const string &name) const {
  auto itr = app_plugins_map.find(name);
  if (itr == app_plugins_map.end())
    throw(std::runtime_error("unable to find plugin: " + name));

  if(itr->second->getState() == AbstractPlugin::plugin_state::initialized) {
    return itr->second.get();
  } else {
    throw(std::runtime_error("The plugin had been registered, but not initialized: " + name));
  }
}

void Application::setId(string_view _id){
  id = _id;
}

const string &Application::getId() const {
  return id;
}

void Application::setRunFlag(){
  running = true;
}

bool Application::isAppRunning() {
  return running;
}

Application &app() {
  return Application::instance();
}
} // namespace appbase
