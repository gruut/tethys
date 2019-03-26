#include "../include/application.hpp"
#include "plugins/block_producer_plugin/include/block_producer_plugin.hpp"
#include "plugins/net_plugin/include/net_plugin.hpp"
#include <iostream>

using namespace std;
using namespace gruut;

int main(int argc, char **argv) {
  if (!appbase::app().initialize<NetPlugin, BlockProducerPlugin>(argc, argv))
    return -1;

  appbase::app().start();
  return 0;
}
