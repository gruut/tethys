#pragma once

#include <vector>
#include <memory>
#include <exception>
#include <unordered_map>
#include <string_view>
#include <typeinfo>

#include <boost/program_options.hpp>
#include <boost/core/demangle.hpp>
#include <boost/asio/io_context.hpp>

#include "plugin.hpp"
#include "../../log/include/log.hpp"

namespace appbase {
  using namespace std;

  namespace po = boost::program_options;

  class ProgramOptions;

  class Application {
  public:
    ~Application();

    template<class ...Plugins>
    bool initialize(int argc, char **argv) {
      try {
        set_program_options();
        if (!parse_program_options(argc, argv))
          return false;

        register_plugins<Plugins...>();

        return initialize_impl();
      } catch (exception &e) {
        logger::ERROR("Initialize failed");
        return false;
      }
    }

    static Application &instance();

  private:
    bool initialize_impl();

    void set_program_options();

    template<class ...Plugins>
    constexpr void register_plugins() {
      (register_plugin<Plugins>(), ...);
    }

    template<typename Plugin>
    void register_plugin() {
      auto name = boost::core::demangle(typeid(Plugin).name());
      app_plugins_map.try_emplace(name, make_unique<Plugin>());
    }

    Application();

    shared_ptr<boost::asio::io_context> io_context;
    unordered_map<string, unique_ptr<AbstractPlugin>> app_plugins_map;
    vector<unique_ptr<AbstractPlugin>> running_plugins;
    unique_ptr<ProgramOptions> program_options;

    bool parse_program_options(int argc, char **argv);

    void initialize_plugins();
  };

  Application &app();
}
