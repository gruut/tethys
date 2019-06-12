#define BOOST_TEST_MODULE runner_test

#include <iostream>
#include <boost/test/included/unit_test.hpp>
#include "../../include/engine.hpp"
#include "../include/dummy_storage.hpp"
#include "../../../plugins/chain_plugin/structure/block.hpp"

BOOST_AUTO_TEST_SUITE(runner_test)

BOOST_AUTO_TEST_CASE(simple_run) {

  tethys::tsce::ContractEngine test_engine;
  test_engine.attachReadInterface(read_storage_interface);

  nlohmann::json msg_block = R"({
    "btime": "1559192461",
    "block": {
      "id": "testblock_id",
      "time": "1559191461",
      "world": "TETHYS19",
      "chain": "SEOUL@KR",
      "height": "2",
      "previd": "",
      "link": "",
      "hash": ""
    },
    "tx": [],
    "state": {
      "txroot": "dummy_value",
      "usroot": "dummy_value",
      "csroot": "dummy_value",
      "sgroot": "dummy_value"
    },
    "signer" : [
      {
        "id" : "5g9CMGLSXbNAKJMbWqBNp7rm78BJCMKhLzZVukBNGHSF",
        "sig" : "dummy_sig"
      }
    ],
    "certificate" : [],
    "producer" : {
      "id": "5g9CMGLSXbNAKJMbWqBNp7rm78BJCMKhLzZVukBNGHSF",
      "sig": "dummy_sig"
    }
  })"_json;

  nlohmann::json msg_tx_1 = R"({
    "txid": "5g9CMGLSXbNAKJMbWqBNp7rm78BJCMKhLzZVukBNGHSF",
    "time": "1559191460",
    "body": {
      "cid": "VALUE-TRANSFER::5g9CMGLSXbNAKJMbWqBNp7rm78BJCMKhLzZVukBNGHSF::SEOUL@KR::TETHYS19",
      "receiver": "5g9CMGLSXbNAKJMbWqBNp7rm78BJCMKhLzZVukBNGHSF",
      "fee": "20",
      "input": [
        [
          {"amount": "100"},
          {"unit": "THY"},
          {"pid": "8CJ8YhBwwgNGKAdzGl1qkKstJi+rUQ7ow8gMHIF3RHU="},
          {"tag": ""}
        ]
      ]
    },
    "user": {
      "id": "5g9CMGLSXbNAKJMbWqBNp7rm78BJCMKhLzZVukBNGHSF",
      "pk": "",
      "a": "",
      "z": ""
    },
    "endorser": [
      {
        "id": "5g9CMGLSXbNAKJMbWqBNp7rm78BJCMKhLzZVukBNGHSF",
        "pk": "",
        "a": "",
        "z": ""
      }
    ]
  })"_json;

  nlohmann::json msg_tx_2 = R"({
    "txid": "5g9CMGLSXbNAKJMbWqBNp7rm78BJCMKhLzZVukBNGHSF",
    "time": "1559191460",
    "body": {
      "cid": "VALUE-TRANSFER::5g9CMGLSXbNAKJMbWqBNp7rm78BJCMKhLzZVukBNGHSF::SEOUL@KR::TETHYS19",
      "receiver": "5g9CMGLSXbNAKJMbWqBNp7rm78BJCMKhLzZVukBNGHSF",
      "fee": "20",
      "input": [
        [
          {"amount": "100"},
          {"unit": "THY"},
          {"pid": "8CJ8YhBwwgNGKAdzGl1qkKstJi+rUQ7ow8gMHIF3RHU="},
          {"tag": ""}
        ]
      ]
    },
    "user": {
      "id": "5g9CMGLSXbNAKJMbWqBNp7rm78BJCMKhLzZVukBNGHSF",
      "pk": "",
      "a": "",
      "z": ""
    },
    "endorser": [
      {
        "id": "5g9CMGLSXbNAKJMbWqBNp7rm78BJCMKhLzZVukBNGHSF",
        "pk": "",
        "a": "",
        "z": ""
      }
    ]
  })"_json;

  // time = 2019-05-30T13:44:20+09:00

  msg_block["tx"].emplace_back(TypeConverter::encodeBase<64>(nlohmann::json::to_cbor(msg_tx_1)));
  msg_block["tx"].emplace_back(TypeConverter::encodeBase<64>(nlohmann::json::to_cbor(msg_tx_2)));

  tethys::Block block;
  block.initialize(msg_block);

  auto result_query = test_engine.procBlock(block);

  if(!result_query) {
    BOOST_TEST(false);
  }

  std::cout << result_query.value() << std::endl;

  BOOST_TEST(true);
}

BOOST_AUTO_TEST_SUITE_END()