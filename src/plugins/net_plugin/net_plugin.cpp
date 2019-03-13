#include "include/net_plugin.hpp"

class NetPluginImpl {
public:
  NetPluginImpl() : signer_conn_table(std::make_shared<SignerConnTable>()),
                    broadcast_check_table(std::make_shared<BroadcastMsgTable>()) {
    //TODO : 자신의 ID를 인증서버로 부터 받아 올 수 있을 떄 수정 될 것.
    Node my_node(Hash<160>::sha1(MY_ID), MY_ID, IP_ADDRESS, DEFAULT_PORT_NUM);
    routing_table = std::make_shared<RoutingTable>(my_node, KBUCKET_SIZE);
  }

  NetPluginImpl(NetPluginImpl
                const &) = delete;

  NetPluginImpl &operator=(NetPluginImpl &) = delete;

  NetPluginImpl(NetPluginImpl
                &&) = default;

  NetPluginImpl &operator=(NetPluginImpl &&) = default;

  ~NetPluginImpl() = default;

  void setUp();

  void bootStrap();

  void run();

private:
  std::shared_ptr<SignerConnTable> signer_conn_table;
  std::shared_ptr<RoutingTable> routing_table;
  std::shared_ptr<BroadcastMsgTable> broadcast_check_table;

  RpcClient rpc_client;
  HttpClient http_client;
  RpcServer rpc_server;

  void pingTask(const Node &node);

  void findNeighborsTask(const IdType &id, const HashedIdType &hashed_id);

  void refreshBuckets();

  void scheduleRefreshBuckets();

  void refreshBroadcastTable();

  void getNodeInfoFromTracker(int max_node = MAX_NODE_INFO_FROM_TRACKER);

};

NetPlugin::NetPlugin() : impl(make_unique<NetPluginImpl>()) {}

void NetPlugin::plugin_initialize() {
  logger::INFO("NetPlugin Initialize");

  temp_channel_handler = app().get_channel<channels::temp_channel::channel_type>().subscribe([](TempData d) {
    std::cout << "NetPlugin handler" << std::endl;
  });
}

