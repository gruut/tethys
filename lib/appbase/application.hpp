#pragma once

#include "abstract_plugin.hpp"
#include <vector>
#include <memory>
#include <map>
#include <string_view>
#include <boost/program_options.hpp>

namespace appbase {
  using namespace std;
  namespace po = boost::program_options;

  class Application {
  public:
    ~Application();

    bool initialize(int argc, char **argv) {
      return initialize_impl(argc, argv);
    }

    static Application &instance();

  private:
    bool initialize_impl(int argc, char **argv);
    void set_program_options();
    bool register_mandatory_plugins();

    Application();

    vector<unique_ptr<AbstractPlugin>> plugins;
    po::options_description program_options;
  };

  Application &app();

  template<typename PluginName>
  class Plugin : public AbstractPlugin {
  public:
  private:
  };
}
