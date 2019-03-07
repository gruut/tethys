#include "../include/application.hpp"
#include "../../../src/plugins/net_plugin/include/net_plugin.hpp"

int main(int argc, char** argv) {
  if(!appbase::app().initialize<NetPlugin>(argc, argv))
    return -1;

  return 0;
}
