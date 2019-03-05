#include <iostream>
#include "application.hpp"

namespace appbase {
  Application::Application() {}

  Application &Application::instance() {
    static Application app;
    return app;
  }

  bool Application::initialize_impl(int argc, char **argv) {
    register_mandatory_plugins();

    set_program_options();

    po::variables_map options_map;
    po::store(po::parse_command_line(argc, argv, program_options), options_map);

    if(options_map.count("help")) {
      cout << program_options << endl;
      return false;
    }
    // Read a config file and concat mandatory_plugins

    return true;
  }

  void Application::set_program_options() {
    po::options_description app_cfg_opts("Application Config Options");
    po::options_description app_cli_opts("Application Command Line Options");

    app_cfg_opts.add_options()
            ("plugin", po::value<vector<string>>()->composing(),
             "You can specify multiple times.");

    app_cli_opts.add_options()
            ("config-path, c", po::value<string>()->default_value("config.ini"),
             "Configuration file path (relative/absolute file path)")
            ("help, h", "Print help message");

    program_options.add(app_cfg_opts);
    program_options.add(app_cli_opts);
  }

  bool Application::register_mandatory_plugins() {
  }

  Application::~Application() = default;

  Application &app() { return Application::instance(); }
}
