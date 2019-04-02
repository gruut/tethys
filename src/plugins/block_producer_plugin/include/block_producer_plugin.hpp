#pragma once

#include <iostream>

#include "../../../../lib/log/include/log.hpp"
#include "../../channel_interface/include/channel_interface.hpp"
#include "../../net_plugin/include/net_plugin.hpp"
#include "application.hpp"
#include "plugin.hpp"

using namespace appbase;

namespace gruut {
class BlockProducerPlugin : public Plugin<BlockProducerPlugin> {
public:
  PLUGIN_REQUIRES((NetPlugin))

  void pluginInitialize(const boost::program_options::variables_map &options) {
    logger::INFO("BlockProducerPlugin Initialize");
  }

  void pluginStart() {
    logger::INFO("BlockProducerPlugin Start");
  }

  void pluginShutdown() {
    logger::INFO("BlockProducerPlugin Shutdown");
  }

  void setProgramOptions(po::options_description &) {
    // do nothing
  }

private:
};
} // namespace gruut
