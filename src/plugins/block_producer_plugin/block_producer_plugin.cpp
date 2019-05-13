#include "include/block_producer_plugin.hpp"

namespace gruut {

class BlockProducerPluginImpl {
public:
  
};

void BlockProducerPlugin::pluginInitialize(const boost::program_options::variables_map &options) {
  logger::INFO("BlockProducerPlugin Initialize");
}

void BlockProducerPlugin::pluginStart() {
  logger::INFO("BlockProducerPlugin Start");
}

BlockProducerPlugin::BlockProducerPlugin() : impl(make_unique<BlockProducerPluginImpl>()) {}

BlockProducerPlugin::~BlockProducerPlugin() {
  impl.reset();
}
} // namespace gruut
