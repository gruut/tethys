#include <iostream>
#include <boost/filesystem.hpp>
#include <boost/asio/signal_set.hpp>

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

  Application::Application() : program_options(make_unique<ProgramOptions>()),
                               io_context_ptr(make_shared<boost::asio::io_context>()) {}

  Application::~Application() = default;

  Application &Application::instance() {
    static Application app;
    return app;
  }

  bool Application::initializeImpl() {
    initializePlugins();

    return true;
  }

  void Application::setProgramOptions() {
    po::options_description app_cfg_opts("Application Config Options");
    po::options_description app_cli_opts("Application Command Line Options");

    app_cfg_opts.add_options()
            ("plugin", po::value<vector<string>>()->composing(),
             "You can specify multiple times.");
    app_cfg_opts.add_options()
            ("p2p-address", po::value<string>()->composing());
    app_cfg_opts.add_options()
            ("tracker-address", po::value<string>()->composing());

    app_cli_opts.add_options()
            ("config-path,c", po::value<string>()->default_value("config.ini"),
             "Configuration file path (relative/absolute file path)")
            ("help,h", "Print help message");

    program_options->config_options.add(app_cfg_opts);
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
    po::store(po::parse_config_file<char>(config_path.make_preferred().string().c_str(),
                                          program_options->config_options), program_options->options_map);

    return true;
  }

  void Application::initializePlugins() {
    auto plugin_names = program_options->options_map.at("plugin").as<vector<string>>();

    for_each(begin(plugin_names), end(plugin_names), [this](const string &plugin_name) {
      auto it = app_plugins_map.find(plugin_name);
      if (it != app_plugins_map.end()) {
        it->second->initialize(program_options->options_map);
        initialized_plugins.push_back(it->second);
      }
    });
  }

  void Application::start() {
    for (auto &running_plugin : initialized_plugins) {
      running_plugin->start();
    }

    shared_ptr<boost::asio::signal_set> sigint_set(new boost::asio::signal_set(*io_context_ptr, SIGINT));
    sigint_set->async_wait([sigint_set,this](const boost::system::error_code& err, int num) {
      logger::ERROR("SIGINT Received: {}", err.message());
      sigint_set->cancel();
      quit();
    });

    shared_ptr<boost::asio::signal_set> sigterm_set(new boost::asio::signal_set(*io_context_ptr, SIGTERM));
    sigterm_set->async_wait([sigterm_set,this](const boost::system::error_code& err, int num) {
      logger::ERROR("SIGTERM Received: {}", err.message());
      sigterm_set->cancel();
      quit();
    });

    shared_ptr<boost::asio::signal_set> sigpipe_set(new boost::asio::signal_set(*io_context_ptr, SIGPIPE));
    sigpipe_set->async_wait([sigpipe_set,this](const boost::system::error_code& err, int num) {
      logger::ERROR("SIGPIPE Received: {}", err.message());
      sigpipe_set->cancel();
      quit();
    });

    io_context_ptr->run();

    shutdown();
  }

  void Application::quit() {
    io_context_ptr->stop();
  }

  void Application::shutdown() {
    logger::INFO("Application Shutdown");

    initialized_plugins.clear();
    io_context_ptr->reset();
  }

  Application &app() { return Application::instance(); }
}
