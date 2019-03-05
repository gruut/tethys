#include "../application.hpp"

int main(int argc, char** argv) {
  if(!appbase::app().initialize(argc, argv))
    return -1;

  return 0;
}