#pragma once

#include "abstract_plugin.hpp"
#include <vector>
#include <memory>
#include <map>
#include <unordered_map>
#include <string_view>
#include <boost/program_options.hpp>

namespace appbase {
  using namespace std;

  namespace po = boost::program_options;

  class ProgramOptions;

  class Application {
  public:
    ~Application();

    bool initialize(int argc, char **argv) {
      return initialize_impl(argc, argv);
    }

    static Application &instance();

  private:
    bool initialize_impl(int argc, char **argv);
    void set_program_options();

    Application();

    //unordered_map<string, unique_ptr<AbstractPlugin>> plugins_map;
    unique_ptr<ProgramOptions> program_options;

    bool parse_program_options(int argc, char **argv);

    void initialize_plugins();
  };

  Application &app();
}
