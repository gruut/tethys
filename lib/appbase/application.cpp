#include <iostream>
#include <boost/filesystem.hpp>
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

  bool Application::initialize_impl() {
    initialize_plugins();

    return true;
  }

  void Application::set_program_options() {
    po::options_description app_cfg_opts("Application Config Options");
    po::options_description app_cli_opts("Application Command Line Options");

    app_cfg_opts.add_options()
            ("plugin", po::value<vector<string>>()->composing(),
             "You can specify multiple times.");

    app_cli_opts.add_options()
            ("config-path,c", po::value<string>()->default_value("config.ini"),
             "Configuration file path (relative/absolute file path)")
            ("help,h", "Print help message");

    program_options->config_options.add(app_cfg_opts);
    program_options->options_description.add(app_cfg_opts);
    program_options->options_description.add(app_cli_opts);
  }

  bool Application::parse_program_options(int argc, char **argv) {
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

  void Application::initialize_plugins() {
    auto plugin_names = program_options->options_map.at("plugin").as<vector<string>>();

    for_each(begin(plugin_names), end(plugin_names), [this](const string &plugin_name) {
      auto it = app_plugins_map.find(plugin_name);
      if (it != app_plugins_map.end()) {
        it->second->initialize();
        initialized_plugins.push_back(it->second);
      }
    });
  }

  void Application::start() {
    for (auto &running_plugin : initialized_plugins) {
      running_plugin->start();
    }

    io_context_ptr->run();
  }

  Application &app() { return Application::instance(); }
}
