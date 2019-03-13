#pragma once

#include <boost/core/demangle.hpp>
#include <boost/preprocessor/seq/for_each.hpp>
#include <typeinfo>
#include <string>

#define PLUGIN_REQUIRES_VISIT(r, visitor, elem) \
  visitor(app().find_or_register_plugin<elem>());

#define PLUGIN_REQUIRES(PLUGINS) \
  template<typename Lambda> \
  void plugin_requires(Lambda&& l) { \
    BOOST_PP_SEQ_FOR_EACH(PLUGIN_REQUIRES_VISIT, l, PLUGINS) \
  }

namespace appbase {
  using namespace std;

  class AbstractPlugin {
  public:
    enum class plugin_state {
      registered,
      initialized
    };

    virtual void initialize() = 0;

    virtual void start() = 0;

    virtual void shutdown() = 0;

    virtual void register_dependencies() = 0;

    virtual plugin_state get_state() const= 0;

    virtual ~AbstractPlugin() = default;
  };

  template<typename Impl>
  class Plugin : public AbstractPlugin {
  public:
    Plugin() : name(boost::core::demangle(typeid(Impl).name())) {}

    const string &get_name() const {
      return name;
    }

    void initialize() override {
      static_cast<Impl *>(this)->plugin_requires([&](auto &plugin) {
        if (plugin.get_state() == plugin_state::registered)
          plugin.initialize();
      });

      static_cast<Impl *>(this)->plugin_initialize();
      static_cast<Impl *>(this)->state = plugin_state::initialized;
    }

    void start() override {

    }

    void shutdown() override {

    }

    virtual plugin_state get_state() const override {
      return state;
    }

    void register_dependencies() override {
      static_cast<Impl *>(this)->plugin_requires([&](auto &plugin) {});
    }

  protected:
    plugin_state state = plugin_state::registered;
  private:
    const string name;
  };
}
