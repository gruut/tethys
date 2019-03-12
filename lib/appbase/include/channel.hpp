#pragma once

#include <boost/signals2.hpp>

namespace appbase {
  using namespace boost::signals2;

  template<typename Data>
  class Channel {
  public:
    class Handle {
    public:
      ~Handle() {
        unsubscribe();
      }

      Handle(const Handle &) = delete;
      Handle &operator=(const Handle &) = delete;

      void unsubscribe() {
        if (handle.connected()) {
          handle.disconnect();
        }
      }

    private:
      using handle_type = connection;

      explicit Handle(handle_type &&handle) : handle(std::move(handle)) {}

      handle_type handle;

      friend class Channel;
    };

    template<typename Callback>
    Handle subscribe(Callback callback) {
      return Handle(s.connect(callback));
    }

    void publish(Data& data) {
      if(has_subscribers()) {
        s(data);
      }
    }

  private:
    bool has_subscribers() {
      return s.num_slots() > 0;
    }

    signal<void(const Data &)> s;
  };
}
