#include "../include/application.hpp"
#include "plugins/block_producer_plugin/include/block_producer_plugin.hpp"
#include "plugins/chain_plugin/include/chain_plugin.hpp"
#include "plugins/net_plugin/include/net_plugin.hpp"
#include "plugins/admin_plugin/include/admin_plugin.hpp"
#include <iostream>

using namespace std;
using namespace tethys;

int main(int argc, char **argv) {
  if (!appbase::app().initialize<AdminPlugin, NetPlugin, ChainPlugin, BlockProducerPlugin>(argc, argv))
    return -1;

  appbase::app().start();
  return 0;
}
