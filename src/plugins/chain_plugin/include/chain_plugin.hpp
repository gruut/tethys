#pragma once

#include "../../../../include/json.hpp"
#include "../../../../lib/appbase/include/application.hpp"
#include "../../../../lib/log/include/log.hpp"
#include "../../../../lib/tethys-utils/src/ags.hpp"
#include "../../../../lib/tethys-utils/src/bytes_builder.hpp"
#include "../../../../lib/tethys-utils/src/sha256.hpp"
#include "../../../../lib/tethys-utils/src/type_converter.hpp"
#include "../../channel_interface/include/channel_interface.hpp"
#include "../structure/block.hpp"
#include "../structure/transaction.hpp"
#include "chain.hpp"
#include "plugin.hpp"
#include "transacton_pool.hpp"

#include <boost/filesystem.hpp>
#include <boost/program_options/options_description.hpp>
#include <boost/program_options/variables_map.hpp>

#include <map>
#include <shared_mutex>

using namespace appbase;

namespace tethys {
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

  Chain &chain();
  TransactionPool &transactionPool();

private:
  std::unique_ptr<class ChainPluginImpl> impl;
};
} // namespace tethys
