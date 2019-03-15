#pragma once

#include "../rpc_services/include/rpc_services.hpp"
#include "signer_conn_manager.hpp"
#include "../kademlia/include/routing.hpp"
#include "../config/include/network_type.hpp"
#include <string>

using namespace std;

namespace gruut {
  namespace net_plugin {
    class RpcServer {
    public:
      RpcServer(const string&, const string&);

      ~RpcServer() {
        server->Shutdown();
        completion_queue->Shutdown();
      }

      void start();
    private:
      void initialize();
      void register_services();

      string address;
      string port_num;

      unique_ptr<Server> server;
      unique_ptr<ServerCompletionQueue> completion_queue;

      GruutGeneralService::AsyncService general_service;

      KademliaService::AsyncService kademlia_service;
    };
  } //namespace net_plugin
} //namespace gruut
