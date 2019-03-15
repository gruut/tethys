#include <thread>
#include <memory>

#include "include/rpc_server.hpp"
#include "include/signer_conn_manager.hpp"
#include "kademlia/include/routing.hpp"
#include "config/include/network_type.hpp"
#include "../../../lib/log/include/log.hpp"

namespace gruut {
  namespace net_plugin {
    RpcServer::RpcServer(const string &addr, const string &port) : address(addr), port_num(port) {}

    void RpcServer::start() {
      void *tag;
      bool ok;

      while (true) {
        GPR_ASSERT(completion_queue->Next(&tag, &ok));
        if (ok)
          static_cast<CallData *>(tag)->proceed();
        else
          std::this_thread::sleep_for(std::chrono::milliseconds(5));
      }
    }
  }
}
