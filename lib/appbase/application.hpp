#pragma once

#include <vector>
#include <memory>
#include <unordered_map>
#include <string_view>
#include <typeinfo>

#include <boost/program_options.hpp>
#include <boost/core/demangle.hpp>

#include "plugin.hpp"

namespace appbase {
  using namespace std;

  namespace po = boost::program_options;

  class ProgramOptions;

  class Application {
  public:
    ~Application();

    template<class ...Plugins>
    bool initialize(int argc, char **argv) {
      set_program_options();
      if (!parse_program_options(argc, argv))
        return false;

      register_plugins<Plugins...>();
      return initialize_impl();
    }

    static Application &instance();

  private:
    bool initialize_impl();

    void set_program_options();

    template <class ...Plugin>
    constexpr void register_plugins() {
      (register_plugin<Plugin>(), ...);
    }

    template<typename Plugin>
    void register_plugin() {
      auto name = boost::core::demangle(typeid(Plugin).name());
      app_plugins_map.try_emplace(name, make_unique<Plugin>());
    }

    Application();

    unordered_map<string, unique_ptr<AbstractPlugin>> app_plugins_map;
    vector<unique_ptr<AbstractPlugin>> running_plugins;
    unique_ptr<ProgramOptions> program_options;

    bool parse_program_options(int argc, char **argv);

    void initialize_plugins();
  };

  Application &app();
}
