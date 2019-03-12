#include <iostream>
#include "../include/application.hpp"
#include "../../../src/plugins/net_plugin/include/net_plugin.hpp"
#include "../../../src/plugins/block_producer_plugin/include/block_producer_plugin.hpp"

using namespace std;

int main(int argc, char** argv) {
  if(!appbase::app().initialize<NetPlugin, BlockProducerPlugin>(argc, argv))
    return -1;

  appbase::app().start();
  return 0;
}
