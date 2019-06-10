#include "include/tx_parallelizer.hpp"

namespace tethys::tsce {
  std::vector <std::vector<TransactionJson>> TxParallelizer::parallelize(BlockJson &block) {

    std::vector <std::vector<TransactionJson>> independ_txs;
    std::vector <std::vector<std::string>> independ_user;
    std::vector <std::vector<std::string>> independ_recv;

    int num_tx = block["tx"].size();

    for (int i = 0; i < num_tx; ++i) {

      TransactionJson this_tx = nlohmann::json::from_cbor(
              TypeConverter::decodeBase<64>(block["tx"][i].get<std::string>()));
      auto user = this_tx["user"]["id"].get<std::string>();
      auto receiver = this_tx["body"]["receiver"].get<std::string>();

      int user_related_bin = -1;
      int recv_related_bin = -1;

      for (int j = 0; j < independ_user.size(); ++j) {
        for (auto &each_user : independ_user[j]) {
          if (each_user == user) {
            user_related_bin = j;
            break;
          }
        }

        if (user_related_bin >= 0)
          break;
      }

      for (int j = 0; j < independ_recv.size(); ++j) {
        for (auto &each_receiver : independ_recv[j]) {
          if (each_receiver == receiver) {
            recv_related_bin = j;
            break;
          }
        }

        if (recv_related_bin >= 0)
          break;
      }

      if (user_related_bin >= 0 && recv_related_bin >= 0) {

        // move receiver-related bin to user-relative bin
        std::copy(independ_user[recv_related_bin].begin(), independ_user[recv_related_bin].end(),
                  std::back_inserter(independ_user[user_related_bin]));
        independ_user[recv_related_bin].clear();

        std::copy(independ_recv[recv_related_bin].begin(), independ_recv[recv_related_bin].end(),
                  std::back_inserter(independ_recv[user_related_bin]));
        independ_recv[user_related_bin].clear();

        std::copy(independ_txs[recv_related_bin].begin(), independ_txs[recv_related_bin].end(),
                  std::back_inserter(independ_txs[user_related_bin]));
        independ_txs[user_related_bin].clear();

        // assign to user_related_bin
        independ_user[user_related_bin].push_back(user);
        independ_recv[user_related_bin].push_back(receiver);
        independ_txs[user_related_bin].push_back(this_tx);

      } else if (user_related_bin >= 0) {

        independ_user[user_related_bin].push_back(user);
        independ_recv[user_related_bin].push_back(receiver);
        independ_txs[user_related_bin].push_back(this_tx);

      } else if (recv_related_bin >= 0) {

        independ_user[recv_related_bin].push_back(user);
        independ_recv[recv_related_bin].push_back(receiver);
        independ_txs[recv_related_bin].push_back(this_tx);

      } else {

        std::vector <std::string> new_user_group;
        new_user_group.push_back(user);
        independ_user.push_back(new_user_group);

        std::vector <std::string> new_receiver_group;
        new_receiver_group.push_back(receiver);
        independ_recv.push_back(new_receiver_group);

        std::vector <TransactionJson> new_tx_group;
        new_tx_group.push_back(this_tx);
        independ_txs.push_back(new_tx_group);

      }
    }

    // independ_txs -> parall_txs

    std::vector <std::vector<TransactionJson>> parall_txs;
    parall_txs.resize(NUM_GSCE_THREAD);

    for (auto &each_group : independ_txs) {

      int min_bin = -1;
      int min_num_tx_bin = num_tx;
      for (int j = 0; j < parall_txs.size(); ++j) {
        if (parall_txs[j].size() < min_num_tx_bin) {
          min_num_tx_bin = parall_txs[j].size();
          min_bin = j;
        }
      }

      if (min_bin < 0)
        min_bin = 0;

      std::copy(each_group.begin(), each_group.end(), std::back_inserter(parall_txs[min_bin]));
    }

    return parall_txs;
  }
}
