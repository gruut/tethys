#include "include/net_plugin.hpp"
#include "include/rpc_client.hpp"
#include "include/rpc_server.hpp"
#include "include/signer_conn_manager.hpp"
#include "kademlia/include/hash.hpp"
#include "kademlia/include/node.hpp"
#include "kademlia/include/routing.hpp"
#include "rpc_services/include/rpc_services.hpp"
#include "config/include/network_config.hpp"
#include "config/include/message.hpp"
#include "config/include/network_type.hpp"

#include "../../../include/json.hpp"
#include <unordered_map>

namespace gruut {
  using namespace std;
  using namespace nlohmann;
  using namespace net_plugin;

  class NetPluginImpl {
  public:
    NetPluginImpl() : signer_conn_table(make_shared<SignerConnTable>()),
                      broadcast_check_table(make_shared<BroadcastMsgTable>()) {
      // TODO : 자신의 ID를 인증서버로 부터 받아 올 수 있을 떄 수정 될 것.
      Node my_node(Hash<160>::sha1(MY_ID), MY_ID, IP_ADDRESS, DEFAULT_PORT_NUM);
      routing_table = make_shared<RoutingTable>(my_node, KBUCKET_SIZE);
    }

    NetPluginImpl(NetPluginImpl const &) = delete;

    NetPluginImpl &operator=(NetPluginImpl &) = delete;

    NetPluginImpl(NetPluginImpl &&) = default;

    NetPluginImpl &operator=(NetPluginImpl &&) = default;

    ~NetPluginImpl() = default;

    void setUp() {
      rpc_server.setUp(signer_conn_table, routing_table, broadcast_check_table);
      rpc_client.setUp(broadcast_check_table);
    }

    void run() {
      rpc_server.run(DEFAULT_PORT_NUM);
    }

  private:
    shared_ptr<SignerConnTable> signer_conn_table;
    shared_ptr<RoutingTable> routing_table;
    shared_ptr<BroadcastMsgTable> broadcast_check_table;

    RpcClient rpc_client;
    RpcServer rpc_server;

    void pingTask(const Node &node) {
      auto endpoint = node.getEndpoint();
      PongData pong = rpc_client.pingReq(endpoint.address, endpoint.port);
      if (!pong.status.ok()) {
        bool evicted = routing_table->peerTimedOut(node);
        if (!evicted)
          pingTask(node);
      }
    }

    void findNeighborsTask(const IdType &id, const HashedIdType &hashed_id) {
      auto target_list = routing_table->findNeighbors(hashed_id, PARALLELISM_ALPHA);

      for (auto &target : target_list) {
        auto endpoint = target.getEndpoint();

        NeighborsData recv_data = rpc_client.findNodeReq(endpoint.address, endpoint.port, id);
        if (!recv_data.status.ok()) {
          bool evicted = routing_table->peerTimedOut(target);
          if (!evicted)
            findNeighborsTask(id, hashed_id);
          else
            return;
        } else {
          for (auto node : recv_data.neighbors) {
            routing_table->addPeer(move(node));
          }
        }
      }
    }

    void refreshBuckets() {
      for (auto bucket = routing_table->begin(); bucket != routing_table->end(); bucket++) {
        auto since_last_update = bucket->timeSinceLastUpdated();

        if (since_last_update > BUCKET_INACTIVE_TIME_BEFORE_REFRESH) {
          if (!bucket->empty()) {
            const auto &node = bucket->selectRandomNode();
            const auto &id = node.getId();
            const auto &hashed_id = node.getIdHash();

            findNeighborsTask(id, hashed_id);
          }
        }
      }
    }

    void scheduleRefreshBuckets() {
      static auto bucket_index = 0U;
      auto bucket = routing_table->cbegin();

      if (bucket_index < routing_table->bucketsCount()) {
        advance(bucket, bucket_index);
        if (!bucket->empty()) {
          const auto &least_recent_node = bucket->leastRecentlySeenNode();
          pingTask(least_recent_node);
        }
        ++bucket_index;
      } else {
        bucket_index = 0;
      }

      refreshBuckets();
    }

    void refreshBroadcastTable() {
      //TODO: gruut util의  Time객체 이용할 것.
      uint64_t now = static_cast<uint64_t>(
              std::chrono::duration_cast<std::chrono::seconds>(
                      std::chrono::system_clock::now().time_since_epoch())
                      .count());

      for (auto it = broadcast_check_table->cbegin(); it != broadcast_check_table->cend();) {
        if (abs((int) (now - it->second)) > KEEP_BROADCAST_MSG_TIME) {
          it = broadcast_check_table->erase(it);
        } else {
          ++it;
        }
      }
    }
  };

  NetPlugin::NetPlugin() : impl(new NetPluginImpl()) {}

  void NetPlugin::pluginInitialize() {
    logger::INFO("NetPlugin Initialize");

    temp_channel_handler = app().getChannel<channels::temp_channel::channel_type>().subscribe(
            [](TempData d) { cout << "NetPlugin handler" << endl; });
  }

  void NetPlugin::pluginStart() {
    logger::INFO("NetPlugin Startup");

    impl->setUp();
  }

  NetPlugin::~NetPlugin() {
    impl.reset();
  }
}
