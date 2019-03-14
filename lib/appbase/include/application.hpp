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
#include "channel.hpp"
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

    void start();

    template<typename ChannelType>
    auto &get_channel() {
      auto key = type_index(typeid(ChannelType));

      auto[channel_it, _] = channels.try_emplace(key, std::make_shared<ChannelType>(io_context_ptr));

      auto channel_ptr = channel_it->second.get();
      return *dynamic_cast<ChannelType *>(channel_ptr);
    }

    template<typename Plugin>
    auto &find_or_register_plugin() {
      auto name = boost::core::demangle(typeid(Plugin).name());
      auto ns_removed_name = name.substr(name.find_last_of(':') + 1);

      auto plug_itr = app_plugins_map.find(ns_removed_name);
      if (plug_itr != app_plugins_map.end()) {
        return *plug_itr->second.get();
      } else {
        register_plugin<Plugin>();

        return *app_plugins_map[ns_removed_name];
      }
    }

    auto &get_io_context() {
      return *io_context_ptr;
    }

    static Application &instance();

  private:
    bool initialize_impl();

    void set_program_options();

    template<typename Plugin>
    void register_plugin() {
      auto name = boost::core::demangle(typeid(Plugin).name());
      auto ns_removed_name = name.substr(name.find_last_of(':') + 1);

      auto[plugins_it, not_existing] = app_plugins_map.try_emplace(ns_removed_name, make_shared<Plugin>());

      if (!not_existing)
        return;

      plugins_it->second.get()->register_dependencies();
    }

    template<class ...Plugins>
    constexpr void register_plugins() {
      (register_plugin<Plugins>(), ...);
    }

    Application();

    shared_ptr<boost::asio::io_context> io_context_ptr;

    unordered_map<string, shared_ptr<AbstractPlugin>> app_plugins_map;
    unordered_map<std::type_index, shared_ptr<AbstractChannel>> channels;

    vector<shared_ptr<AbstractPlugin>> initialized_plugins;
    unique_ptr<ProgramOptions> program_options;

    bool parse_program_options(int argc, char **argv);

    void initialize_plugins();
  };

  Application &app();
}
