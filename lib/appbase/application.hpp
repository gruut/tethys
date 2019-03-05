#ifndef GRUUT_APPLICATION_HPP
#define GRUUT_APPLICATION_HPP

#include <memory>

namespace appbase {
  class ApplicationImpl;

  class Application {
  public:
    ~Application();

    static Application& instance();
  private:
    std::unique_ptr<ApplicationImpl> app_impl;

    Application();
  };

  Application& app();
}

#endif //GRUUT_APPLICATION_HPP
