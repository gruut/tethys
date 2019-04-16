#include "include/net_plugin.hpp"
#include "../../../lib/json/include/json.hpp"
#include "config/include/network_config.hpp"
#include "include/http_client.hpp"
#include "include/id_mapping_table.hpp"
#include "include/message_builder.hpp"
#include "include/message_packer.hpp"
#include "rpc_services/include/rpc_services.hpp"

#include "../../../lib/gruut-utils/src/hmac.hpp"
#include "../../../lib/gruut-utils/src/random_number_generator.hpp"
#include "../../../lib/gruut-utils/src/sha256.hpp"
#include "../../../lib/gruut-utils/src/time_util.hpp"
#include "../../../lib/gruut-utils/src/type_converter.hpp"

#include <boost/asio/steady_timer.hpp>
#include <exception>
#include <future>
#include <stdexcept>
#include <unordered_map>

namespace gruut {
using namespace std;
using namespace net_plugin;

const auto CONNECTION_CHECK_PERIOD = std::chrono::seconds(30);
const auto NET_MESSAGE_CHECK_PERIOD = std::chrono::milliseconds(1);
const auto BROADCAST_MSG_CHECK_PERIOD = std::chrono::minutes(3);

constexpr unsigned int KBUCKET_SIZE = 20;
constexpr int KEEP_BROADCAST_MSG_TIME = 180; // seconds
constexpr auto FIND_NODE_TIMEOUT = std::chrono::milliseconds(100);

class NetPluginImpl {
public:
  GruutMergerService::AsyncService merger_service;
  GruutSignerService::AsyncService signer_service;
  KademliaService::AsyncService kademlia_service;

  string p2p_address;

  string tracker_address;

  unique_ptr<Server> server;
  unique_ptr<ServerCompletionQueue> completion_queue;

  shared_ptr<SignerPoolManager> signer_pool_manager;
  shared_ptr<SignerConnTable> signer_conn_table;
  shared_ptr<RoutingTable> routing_table;
  shared_ptr<IdMappingTable> id_mapping_table;
  shared_ptr<BroadcastMsgTable> broadcast_check_table;

  unique_ptr<boost::asio::steady_timer> connection_check_timer;
  unique_ptr<boost::asio::steady_timer> net_message_check_timer;

  outgoing::channels::network::channel_type::Handle out_channel_subscription;

  ~NetPluginImpl() {
    if (server != nullptr)
      server->Shutdown();

    if (completion_queue != nullptr)
      completion_queue->Shutdown();
  }

  void initialize() {
    initializeServer();
    initializeRoutingTable();
    registerServices();

    connection_check_timer = make_unique<boost::asio::steady_timer>(app().getIoContext());
    net_message_check_timer = make_unique<boost::asio::steady_timer>(app().getIoContext());
  }

  void initializeRoutingTable() {
    auto [host, port] = getHostAndPort(p2p_address);

    Node my_node(Hash<160>::sha1(p2p_address), p2p_address, host, port);
    routing_table = make_shared<RoutingTable>(my_node, KBUCKET_SIZE);
  }

  void initializeServer() {
    ServerBuilder builder;

    builder.AddListeningPort(p2p_address, grpc::InsecureServerCredentials());
    builder.RegisterService(&signer_service);
    builder.RegisterService(&merger_service);
    builder.RegisterService(&kademlia_service);

    completion_queue = builder.AddCompletionQueue();
    server = builder.BuildAndStart();

    logger::INFO("Rpc Server listening on {}", p2p_address);
  }

  void registerServices() {
    signer_conn_table = make_shared<SignerConnTable>();
    broadcast_check_table = make_shared<BroadcastMsgTable>();

    new OpenChannelWithSigner(&signer_service, completion_queue.get(), signer_conn_table, signer_pool_manager);
    new SignerService(&signer_service, completion_queue.get(), signer_pool_manager);
    new MergerService(&merger_service, completion_queue.get(), routing_table, broadcast_check_table, id_mapping_table);
    new FindNode(&kademlia_service, completion_queue.get(), routing_table);
  }

  void start() {
    monitorCompletionQueue();
    getPeersFromTracker();
    startConnectionMonitors();
  }

  void monitorCompletionQueue() {
    net_message_check_timer->expires_from_now(NET_MESSAGE_CHECK_PERIOD);
    net_message_check_timer->async_wait([this](boost::system::error_code ec) {
      if (!ec) {
        void *tag;
        bool ok;

        gpr_timespec deadline;
        deadline.clock_type = GPR_TIMESPAN;
        deadline.tv_sec = 0;
        deadline.tv_nsec = 5;

        auto queue_state = completion_queue->AsyncNext(&tag, &ok, deadline);
        if (ok && queue_state == CompletionQueue::NextStatus::GOT_EVENT) {
          static_cast<CallData *>(tag)->proceed();
        }
      } else {
        logger::ERROR("Error from net_message_check_timer: {}", ec.message());
      }

      monitorCompletionQueue();
    });
  }

  void getPeersFromTracker() {
    if (!tracker_address.empty()) {
      logger::INFO("Start to get peers list from tracker");

      auto [addr, port] = getHostAndPort(tracker_address);
      auto [my_host, my_port] = getHostAndPort(p2p_address);

      HttpClient http_client(addr, port);

      auto res = http_client.get("/announce", "port=" + my_port);
      if (!res.empty()) {
        logger::INFO("Get a response from a tracker : {}", res);
        auto peers = json::parse(res);

        if (!json::is_empty(peers)) {
          for (auto &peer : peers) {
            auto [peer_host, peer_port] = getHostAndPort(peer);
            auto id = peer_host;

            if (id != getMyNetId()) {
              Node node = Node(Hash<160>::sha1(id), id, peer_host, peer_port);
              routing_table->addPeer(move(node));
            }
          }
        }
      }
    }
  }

  void startConnectionMonitors() {
    connection_check_timer->expires_from_now(CONNECTION_CHECK_PERIOD);
    connection_check_timer->async_wait([this](boost::system::error_code ec) {
      if (!ec) {
        refreshBuckets();
      } else {
        logger::ERROR("Error from connection_check_timer: {}", ec.message());
        startConnectionMonitors();
      }
    });
  }

  void refreshBuckets() {
    logger::INFO("Start to refresh buckets");

    for (auto &bucket : *routing_table) {
      if (!bucket.empty()) {
        auto dead_node_ids = bucket.removeDeadNodes();
        if (dead_node_ids.has_value()) {
          for (auto &dead_hashed_id : dead_node_ids.value()) {
            id_mapping_table->unmapId(dead_hashed_id);
          }
        }
        auto nodes = bucket.selectAliveNodes(false);
        async(launch::async, &NetPluginImpl::findNeighbors, this, nodes);
      }
    }

    startConnectionMonitors();
  }

  void monitorBroadcastMsgTable() {
    connection_check_timer->expires_from_now(BROADCAST_MSG_CHECK_PERIOD);
    connection_check_timer->async_wait([this](boost::system::error_code ec) {
      if (!ec) {
        refreshBroadcastCheckTable();
      } else {
        logger::ERROR("ERROR from boradcast_msg_check_timer: {}", ec.message());
        monitorBroadcastMsgTable();
      }
    });
  }

  void refreshBroadcastCheckTable() {
    logger::INFO("Start to refresh broadcast check table");
    if (!broadcast_check_table->empty()) {
      auto now = TimeUtil::nowBigInt();
      for (auto it = broadcast_check_table->begin(); it != broadcast_check_table->end();) {
        if (abs((int)(now - it->second)) > KEEP_BROADCAST_MSG_TIME)
          it = broadcast_check_table->erase(it);
        else
          ++it;
      }
    }
    monitorBroadcastMsgTable();
  }

  void findNeighbors(vector<Node> nodes) {
    for (auto &node : nodes) {
      auto endpoint = node.getEndpoint();

      NeighborsData recv_data = queryNeighborNodes(endpoint.address, endpoint.port, node.getId(), node.getChannelPtr());
      if (!recv_data.status.ok()) {
        bool evicted = routing_table->peerTimedOut(node);
        if (!evicted) {
          queryNeighborNodes(endpoint.address, endpoint.port, node.getId(), node.getChannelPtr());
        }
      } else {
        for (auto &neighbor : recv_data.neighbors) {
          routing_table->addPeer(move(neighbor));
        }
      }
    }
  }

  template <typename TStub, typename TService>
  unique_ptr<TStub> genStub(shared_ptr<grpc::Channel> channel) {
    return TService::NewStub(channel);
  }

  NeighborsData queryNeighborNodes(const string &receiver_addr, const string &receiver_port, const net_id_type &target_id,
                                   shared_ptr<grpc::Channel> channel) {
    auto stub = genStub<KademliaService::Stub, KademliaService>(channel);

    Target target;
    target.set_target_id(target_id);
    target.set_sender_id(getMyNetId());

    auto [host, port] = getHostAndPort(p2p_address);
    target.set_sender_address(host);
    target.set_sender_port(port);
    target.set_time_stamp(0);

    ClientContext context;
    Neighbors neighbors;

    std::chrono::time_point deadline = std::chrono::system_clock::now() + FIND_NODE_TIMEOUT;
    context.set_deadline(deadline);

    grpc::Status status = stub->FindNode(&context, target, &neighbors);

    vector<Node> neighbor_list;
    for (int i = 0; i < neighbors.neighbors_size(); i++) {
      const auto &node = neighbors.neighbors(i);

      if (getMyNetId() != node.node_id()) {
        neighbor_list.emplace_back(Node(Hash<160>::sha1(node.node_id()), node.node_id(), node.address(), node.port()));
      }
    }

    return NeighborsData{neighbor_list, neighbors.time_stamp(), status};
  }

  string getMyNetId() {
    return p2p_address;
  }

  pair<string, string> getHostAndPort(const string addr) {
    auto host = addr.substr(0, addr.find_first_of(':'));
    auto port = addr.substr(addr.find_last_of(':') + 1);

    return pair<string, string>(host, port);
  }

  void sendMessage(OutNetMsg &out_msg) {
    if (checkMergerMsgType(out_msg.type)) {
      auto packed_msg = MessagePacker::packMessage<MACAlgorithmType::NONE>(out_msg);
      bool is_broadcast(out_msg.receivers.empty());

      grpc_merger::RequestMsg request;
      request.set_message(packed_msg);
      request.set_broadcast(is_broadcast);

      ClientContext context;
      std::chrono::time_point deadline = std::chrono::system_clock::now() + GENERAL_SERVICE_TIMEOUT;
      context.set_deadline(deadline);
      context.AddMetadata("net_id", getMyNetId());

      grpc_merger::MsgStatus msg_status;

      if (is_broadcast) {
        // TODO : broadcast 확인을 위한 msg id를 정하는 방법이 없어 현재 임시.
        int random_num = RandomNumGenerator::getRange(0, 1000);
        auto vec_msg_id = Sha256::hash(TimeUtil::now() + to_string(random_num));
        string str_hash_msg_id(vec_msg_id.begin(), vec_msg_id.end());

        broadcast_check_table->insert({str_hash_msg_id, TimeUtil::nowBigInt()});
        request.set_message_id(str_hash_msg_id);

        for (auto &bucket : *routing_table) {
          if (!bucket.empty()) {
            auto nodes = bucket.selectRandomAliveNodes(PARALLELISM_ALPHA);
            for (auto &n : nodes) {
              auto stub = genStub<GruutMergerService::Stub, GruutMergerService>(n.getChannelPtr());
              auto status = stub->MergerService(&context, request, &msg_status);
            }
          }
        }
      } else {
        for (auto &b58_receiver_id : out_msg.receivers) {
          auto hashed_net_id = id_mapping_table->get(b58_receiver_id);

          if(hashed_net_id.has_value()) {
            auto node = routing_table->findNode(hashed_net_id.value());

            if (node.has_value()) {
              auto stub = genStub<GruutMergerService::Stub, GruutMergerService>(node.value().getChannelPtr());
              auto status = stub->MergerService(&context, request, &msg_status);
            } else {
              id_mapping_table->unmapId(b58_receiver_id);
            }
          }
        }
      }
    } else if (checkSignerMsgType(out_msg.type)) {
      for (auto &b58_receiver_id : out_msg.receivers) {
        SignerRpcInfo signer_rpc_info = signer_conn_table->getRpcInfo(b58_receiver_id);
        if (signer_rpc_info.send_msg == nullptr)
          continue;

        string packed_msg;
        if (out_msg.type == MessageType::MSG_REQ_SSIG) {
          auto hmac_key = signer_pool_manager->getHmacKey(b58_receiver_id);
          if (!hmac_key.has_value())
            continue;
          packed_msg = MessagePacker::packMessage<MACAlgorithmType::HMAC>(out_msg, hmac_key.value());
        } else
          packed_msg = MessagePacker::packMessage<MACAlgorithmType::NONE>(out_msg);

        auto tag = static_cast<Identity *>(signer_rpc_info.tag_identity);
        Request req;
        req.set_message(packed_msg);
        signer_rpc_info.send_msg->Write(req, tag);
      }
    }
  }

  bool checkMergerMsgType(MessageType msg_type) {
    return (msg_type == MessageType::MSG_TX || msg_type == MessageType::MSG_REQ_BLOCK || msg_type == MessageType::MSG_BLOCK);
  }

  bool checkSignerMsgType(MessageType msg_type) {
    return msg_type == MessageType::MSG_REQ_SSIG;
  }
};

NetPlugin::NetPlugin() : impl(new NetPluginImpl()) {}

void NetPlugin::setProgramOptions(options_description &cfg) {
  cfg.add_options()("p2p-address", po::value<string>()->composing());
  cfg.add_options()("tracker-address", po::value<string>()->composing());
}

void NetPlugin::pluginInitialize(const variables_map &options) {
  logger::INFO("NetPlugin Initialize");

  if (options.count("p2p-address") > 0) {
    auto address = options["p2p-address"].as<string>();

    impl->p2p_address = address;
  }

  if (options.count("tracker-address")) {
    auto tracker_address = options["tracker-address"].as<string>();

    impl->tracker_address = tracker_address;
  }

  auto &out_channel = app().getChannel<outgoing::channels::network::channel_type>();
  impl->out_channel_subscription = out_channel.subscribe([this](auto data) { impl->sendMessage(data); });

  impl->initialize();
}

void NetPlugin::pluginStart() {
  logger::INFO("NetPlugin Start");

  impl->start();
}

NetPlugin::~NetPlugin() {
  impl.reset();
}
} // namespace gruut
