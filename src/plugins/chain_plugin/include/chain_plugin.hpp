#pragma once

#include <boost/filesystem.hpp>
#include <boost/program_options/variables_map.hpp>

#include "../../../../lib/log/include/log.hpp"
#include "../../channel_interface/include/channel_interface.hpp"
#include "plugin.hpp"

using namespace appbase;

namespace gruut {
class ChainPlugin : public Plugin<ChainPlugin> {
public:
  PLUGIN_REQUIRES()

  ChainPlugin();

  ~ChainPlugin() override;

  void pluginInitialize(const boost::program_options::variables_map &options);

  void pluginStart() {
    logger::INFO("ChainPlugin Start");
  }

  void pluginShutdown() {
    logger::INFO("ChainPlugin Shutdown");
  }

private:
  std::unique_ptr<class ChainPluginImpl> impl;
};
} // namespace gruut
