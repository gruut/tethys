#pragma once

#include <boost/core/demangle.hpp>
#include <boost/preprocessor/seq/for_each.hpp>
#include <typeinfo>
#include <string>

#define PLUGIN_REQUIRES_VISIT(r, visitor, elem) \
  visitor(app().findOrRegisterPlugin<elem>());

#define PLUGIN_REQUIRES(PLUGINS) \
  template<typename Lambda> \
  void pluginRequires(Lambda&& l) { \
    BOOST_PP_SEQ_FOR_EACH(PLUGIN_REQUIRES_VISIT, l, PLUGINS) \
  }

namespace appbase {
  using namespace std;
  using namespace boost::program_options;

  class AbstractPlugin {
  public:
    enum class plugin_state {
      registered,
      initialized
    };

    virtual void initialize(const variables_map&) = 0;

    virtual void start() = 0;

    virtual void shutdown() = 0;

    virtual void registerDependencies() = 0;

    virtual plugin_state getState() const= 0;

    virtual ~AbstractPlugin() = default;
  };

  template<typename Impl>
  class Plugin : public AbstractPlugin {
  public:
    Plugin() : name(boost::core::demangle(typeid(Impl).name())) {}

    const string &getName() const {
      return name;
    }

    void initialize(const variables_map& options) override {
      static_cast<Impl *>(this)->pluginRequires([&](auto &plugin) {
        if (plugin.getState() == plugin_state::registered)
          plugin.initialize(options);
      });

      static_cast<Impl *>(this)->pluginInitialize(options);
      static_cast<Impl *>(this)->state = plugin_state::initialized;
    }

    void start() override {
      static_cast<Impl *>(this)->pluginStart();
    }

    void shutdown() override {

    }

    virtual plugin_state getState() const override {
      return state;
    }

    void registerDependencies() override {
      static_cast<Impl *>(this)->pluginRequires([&](auto &plugin) {});
    }

  protected:
    plugin_state state = plugin_state::registered;
  private:
    const string name;
  };
}
