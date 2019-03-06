#pragma once
#include <boost/core/demangle.hpp>
#include <typeinfo>
#include <string>

namespace appbase {
  using namespace std;

  class AbstractPlugin {

  };

  template<typename Impl>
  class Plugin : public AbstractPlugin {
  public:
    Plugin() : name(boost::core::demangle(typeid(Impl).name())) {}

  private:
    const string name;
  };
}
