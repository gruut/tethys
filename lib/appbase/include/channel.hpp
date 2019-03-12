#pragma once

#include <boost/signals2.hpp>
#include <boost/asio/io_context.hpp>

namespace appbase {
  using namespace boost::signals2;

  template<typename Data>
  class Channel {
  public:
    Channel(std::shared_ptr<boost::asio::io_context> &p) : ioc_ptr(p) {}

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

    void publish(Data &data) {
      if (has_subscribers()) {
        ioc_ptr->get_executor().post([this, data]() {
          s(data);
        });
      }
    }

  private:
    bool has_subscribers() {
      return s.num_slots() > 0;
    }

    std::shared_ptr<boost::asio::io_context> ioc_ptr;
    signal<void(const Data &)> s;
  };
}
