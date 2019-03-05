#include "application.hpp"

namespace appbase {
  using namespace std;

  class ApplicationImpl {

  };

  Application::Application() : app_impl(std::make_unique<ApplicationImpl>()) {}

  Application &Application::instance() {
    static Application app;
    return app;
  }

  Application::~Application() = default;

  Application &app() { return Application::instance(); }
}
