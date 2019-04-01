#include "include/chain_plugin.hpp"
#include "../../../include/json.hpp"
#include "../../../lib/log/include/log.hpp"

namespace gruut {

namespace fs = boost::filesystem;

class ChainPluginImpl {
public:
  void initialize() {}
};

ChainPlugin::ChainPlugin() : impl(new ChainPluginImpl()) {}

void ChainPlugin::pluginInitialize(const boost::program_options::variables_map &options) {
  logger::INFO("ChainPlugin Initialize");

  if (options.count("genesis-block") > 0) {
    fs::path config_path = options.at("genesis-block").as<std::string>();

    if (config_path.is_relative())
      config_path = fs::current_path() / config_path;

    if (!fs::exists(config_path)) {
      logger::ERROR("Can't find a genesis block state file. (path: {})", config_path.string());
      throw std::invalid_argument("Cannot find a genesis_block.json"s);
    }

    std::ifstream i(config_path.make_preferred().string());
    nlohmann::json genesis_state;

    i >> genesis_state;

    impl->initialize();
  } else {
    logger::ERROR("genesis block state file does not exist");
    throw std::invalid_argument("genesis block state file does not exist"s);
  }
}

ChainPlugin::~ChainPlugin() {
  impl.reset();
}
} // namespace gruut
