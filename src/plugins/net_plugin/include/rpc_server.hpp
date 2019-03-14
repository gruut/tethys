#pragma once

#include "../rpc_services/include/rpc_services.hpp"
#include "signer_conn_manager.hpp"
#include "../kademlia/include/routing.hpp"
#include "../config/include/network_type.hpp"
#include <string>

namespace gruut {
namespace net_plugin {
class RpcServer {
public:
  RpcServer() = default;
  ~RpcServer() {
	m_server->Shutdown();
	m_completion_queue->Shutdown();
  }

  void setUp(std::shared_ptr<SignerConnTable> signer_conn_table,
  			 std::shared_ptr<RoutingTable> routing_table,
  			 std::shared_ptr<BroadcastMsgTable> broadcast_check_table);

  void run(const std::string &port_num);

private:
  std::shared_ptr<SignerConnTable> m_signer_conn_table;
  std::shared_ptr<RoutingTable> m_routing_table;
  std::shared_ptr<BroadcastMsgTable> m_broadcast_check_table;

  std::string m_port_num;
  std::unique_ptr<Server> m_server;
  std::unique_ptr<ServerCompletionQueue> m_completion_queue;

  GruutGeneralService::AsyncService m_general_service;

  KademliaService::AsyncService m_kademlia_service;

  void initService();
  void start();
};
} //namespace net_plugin
} //namespace gruut