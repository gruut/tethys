#pragma once

#include "../../../../include/json.hpp"
#include "../../../../lib/appbase/include/application.hpp"
#include "../../../../lib/gruut-utils/src/ags.hpp"
#include "../../../../lib/gruut-utils/src/bytes_builder.hpp"
#include "../../../../lib/gruut-utils/src/sha256.hpp"
#include "../../../../lib/gruut-utils/src/type_converter.hpp"
#include "../../../../lib/log/include/log.hpp"
#include "../../channel_interface/include/channel_interface.hpp"
#include "../structure/block.hpp"
#include "../structure/transaction.hpp"
#include "chain.hpp"
#include "plugin.hpp"

#include <boost/filesystem.hpp>
#include <boost/program_options/options_description.hpp>
#include <boost/program_options/variables_map.hpp>

#include <map>
#include <shared_mutex>

using namespace appbase;

namespace gruut {
class ChainPlugin : public Plugin<ChainPlugin> {
public:
  PLUGIN_REQUIRES()

  ChainPlugin();

  ~ChainPlugin() override;

  void pluginInitialize(const boost::program_options::variables_map &options);

  void pluginStart();

  void pluginShutdown() {
    logger::INFO("ChainPlugin Shutdown");
  }

  void setProgramOptions(options_description &cfg) override;

  void asyncFetchTransactionsFromPool();

private:
  std::unique_ptr<class ChainPluginImpl> impl;
};
} // namespace gruut
