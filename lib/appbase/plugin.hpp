#pragma once

#include <boost/core/demangle.hpp>
#include <typeinfo>
#include <string>

namespace appbase {
  using namespace std;

  class AbstractPlugin {
  public:
    virtual void initialize() = 0;
    virtual void start() = 0;
    virtual void shutdown() = 0;

    virtual ~AbstractPlugin() = default;
  };

  template<typename Impl>
  class Plugin : public AbstractPlugin {
  public:
    Plugin() : name(boost::core::demangle(typeid(Impl).name())) {}

  private:
    const string name;
  };
}
