#pragma once

#include <iostream>

#include "application.hpp"
#include "plugin.hpp"
#include "channel_interface.hpp"
#include "../../net_plugin/include/net_plugin.hpp"
#include "../../../../lib/log/include/log.hpp"

using namespace appbase;

namespace gruut {
  class BlockProducerPlugin : public Plugin<BlockProducerPlugin> {
  public:
    PLUGIN_REQUIRES((NetPlugin))

    void pluginInitialize(const boost::program_options::variables_map& options) {
      logger::INFO("BlockProducerPlugin Initialize");
    }

    void pluginStart() {
      logger::INFO("BlockProducerPlugin Start");
    }

    void pluginShutdown() {
      logger::INFO("BlockProducerPlugin Shutdown");
    }

  private:
  };
}
