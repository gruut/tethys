#pragma once

#include <exception>
#include <memory>
#include <string_view>
#include <typeinfo>
#include <unordered_map>
#include <vector>

#include <boost/asio/io_context.hpp>
#include <boost/core/demangle.hpp>
#include <boost/program_options.hpp>

#include "../../log/include/log.hpp"
#include "channel.hpp"
#include "plugin.hpp"

namespace appbase {
using namespace std;

namespace po = boost::program_options;

class ProgramOptions;

class Application {
public:
  ~Application();

  template <class... Plugins>
  bool initialize(int argc, char **argv) {
    try {
      registerPlugins<Plugins...>();

      po::options_description plugin_cfg_opts("Plugin Config Options");
      for (auto &[_, plugin] : app_plugins_map) {
        plugin->setProgramOptions(plugin_cfg_opts);
      }

      setProgramOptions(plugin_cfg_opts);
      if (!parseProgramOptions(argc, argv))
        return false;

      return initializeImpl();
    } catch (exception &e) {
      logger::ERROR("Initialize failed: {}", e.what());
      return false;
    }
  }

  void start();

  void quit();

  void shutdown();

  template <typename Channel>
  auto &getChannel() {
    auto key = type_index(typeid(Channel));

    auto [channel_it, _] = channels.try_emplace(key, std::make_shared<typename Channel::channel_type>(io_context_ptr));

    auto channel_ptr = channel_it->second.get();
    return *dynamic_cast<typename Channel::channel_type *>(channel_ptr);
  }

  template <typename Plugin>
  auto &findOrRegisterPlugin() {
    auto name = boost::core::demangle(typeid(Plugin).name());
    auto ns_removed_name = name.substr(name.find_last_of(':') + 1);

    auto plug_itr = app_plugins_map.find(ns_removed_name);
    if (plug_itr != app_plugins_map.end()) {
      return *plug_itr->second.get();
    } else {
      registerPlugin<Plugin>();
      return *app_plugins_map[ns_removed_name];
    }
  }

  void setId(string_view _id);
  void setRunFlag();
  const string &getId() const;

  bool isAppRunning();

  auto &getIoContext() {
    return *io_context_ptr;
  }

  AbstractPlugin* getPlugin(const string &name) const;

  static Application &instance();

private:
  bool initializeImpl();

  void setProgramOptions(po::options_description &);

  template <typename Plugin>
  void registerPlugin() {
    auto name = boost::core::demangle(typeid(Plugin).name());
    auto ns_removed_name = name.substr(name.find_last_of(':') + 1);

    auto [plugins_it, not_existing] = app_plugins_map.try_emplace(ns_removed_name, make_shared<Plugin>());

    if (!not_existing)
      return;

    plugins_it->second.get()->registerDependencies();
  }

  template <class... Plugins>
  constexpr void registerPlugins() {
    (registerPlugin<Plugins>(), ...);
  }

  Application();

  shared_ptr<boost::asio::io_context> io_context_ptr;

  unordered_map<string, shared_ptr<AbstractPlugin>> app_plugins_map;
  unordered_map<std::type_index, shared_ptr<AbstractChannel>> channels;

  unique_ptr<ProgramOptions> program_options;

  string id;

  bool running = false;

private:
  bool parseProgramOptions(int argc, char **argv);

  void initializePlugins();

  void startInitializedPlugins();

  void registerErrorSignalHandlers();

};

Application &app();
} // namespace appbase
