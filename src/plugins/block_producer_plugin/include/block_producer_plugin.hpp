#pragma once

#include <iostream>

#include "../../../../lib/log/include/log.hpp"
#include "../../channel_interface/include/channel_interface.hpp"
#include "../../chain_plugin/include/chain_plugin.hpp"
#include "../../net_plugin/include/net_plugin.hpp"

#include "application.hpp"
#include "plugin.hpp"

using namespace appbase;

namespace tethys {
class BlockProducerPlugin : public Plugin<BlockProducerPlugin> {
public:
  PLUGIN_REQUIRES((ChainPlugin)(NetPlugin))

  BlockProducerPlugin();
  ~BlockProducerPlugin();

  void pluginInitialize(const boost::program_options::variables_map &options);

  void pluginStart();

  void pluginShutdown() {
    logger::INFO("BlockProducerPlugin Shutdown");
  }

  void setProgramOptions(po::options_description &) {
    // do nothing
  }

private:
  std::unique_ptr<class BlockProducerPluginImpl> impl;
};
} // namespace tethys
