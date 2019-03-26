#include "../include/application.hpp"
#include <iostream>

using namespace std;

int main(int argc, char **argv) {
  if (!appbase::app().initialize<>(argc, argv))
    return -1;

  appbase::app().start();
  return 0;
}
