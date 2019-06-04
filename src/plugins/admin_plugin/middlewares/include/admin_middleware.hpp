#pragma once

namespace tethys::admin_plugin {
using namespace std;

using error_message = string;

template <class Service>
class AdminMiddleware {
public:
  pair<bool, error_message> next() {
    // do nothing
    return make_pair(true, "");
  };
};

template <>
class AdminMiddleware<LoginService> {
public:
  pair<bool, error_message> next() {
    if (app().isAppRunning())
      return make_pair(false, "Please sign in after stopping the node");

    return make_pair(true, "");
  }
};

template <>
class AdminMiddleware<SetupService> {
public:
  pair<bool, error_message> next() {
    if (app().isUserSignedIn())
      return make_pair(false, "The node has already been setup");

    return make_pair(true, "");
  }
};

template <>
class AdminMiddleware<LoadWorldService> {
public:
  pair<bool, error_message> next() {
    if (app().isAppRunning())
      return make_pair(false, "If you want to join the world, stop node and then load different world");

    return make_pair(true, "");
  }
};

template <>
class AdminMiddleware<LoadChainService> {
public:
  pair<bool, error_message> next() {
    if (app().isAppRunning())
      return make_pair(false, "If you want to join a different local chain, stop node and then load different local chain");

    return make_pair(true, "");
  }
};
} // namespace tethys::admin_plugin

