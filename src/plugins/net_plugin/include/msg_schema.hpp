#pragma once

#include "../../../../lib/json/include/json-schema.hpp"
#include "../config/include/message.hpp"
#include <map>
#include <vector>

namespace gruut {
namespace net_plugin {

using nlohmann::json_schema_draft4::json_validator;
using SchemaCheckMap = std::map<MessageType, json_validator>;

const auto SCHEMA_PING = R"({
  "title": "Ping",
  "type": "object",
  "properties": {
    "status": {
      "type": "object",
      "properties": {
        "time": {
          "type": "string"
        },
        "world": {
          "type": "string"
        },
        "chain": {
          "type": "string"
        },
        "block": {
          "type": "object",
          "properties": {
            "id": {
              "type": "string"
            },
            "pid": {
              "type": "string"
            },
            "height": {
              "type": "string"
            },
            "hash": {
              "type": "string"
            }
          },
          "required":[
            "id",
            "pid",
            "height",
            "hash"
          ]
        }
      },
      "required":[
        "time",
        "world",
        "chain",
        "block"
      ]
    },
    "merger": {
      "type": "object",
      "properties":{
       "id": {
        "type": "string"
        },
        "sig": {
          "type": "string"
        }
      },
      "required" : [
        "id",
        "sig"
      ]
    }
  },
  "required": [
    "status",
    "merger"
  ]
})"_json;
const auto SCHEMA_REQ_BLOCK = R"({
  "title": "Request block",
  "type": "object",
  "properties": {
    "time": {
      "type": "string"
    },
    "world": {
      "type": "string"
    },
    "chain": {
      "type": "string"
    },
    "block": {
      "type": "object",
      "properties": {
        "id": {
          "type": "string"
        },
        "height": {
          "type": "string"
        }
      },
      "required":[
        "id",
        "height"
      ]
    },
    "merger": {
      "type": "object",
      "properties":{
       "id": {
        "type": "string"
        },
        "sig": {
          "type": "string"
        }
      },
      "required" : [
        "id",
        "sig"
      ]
    }
  },
  "required": [
    "time",
    "world",
    "chain",
    "block",
    "merger"
  ]
})"_json;
const auto SCHEMA_BLOCK = R"({
  "title": "Block",
  "type": "object",
  "properties": {
    "block": {
      "type": "object",
      "properties": {
        "id":{
          "type": "string"
        },
        "time":{
          "type": "string"
        },
        "world":{
          "type": "string"
        },
        "chain":{
          "type": "string"
        },
        "height":{
          "type": "string"
        },
        "pid":{
          "type": "string"
        },
        "hash":{
          "type": "string"
        }
      },
      "required":[
        "id",
        "time",
        "world",
        "chain",
        "height",
        "pid",
        "hash"
      ]
    },
    "tx":{
      "type": "array",
      "items":{
        "type": "string"
      }
    },
	"aggz": {
		"type": "string"
	},
    "state":{
      "type": "object",
      "properties":{
        "txroot":{
          "type": "string"
        },
        "usroot":{
          "type": "string"
        },
        "sgroot":{
          "type": "string"
        }
      },
      "required":[
        "txroot",
        "usroot",
        "sgroot"
      ]
    },
    "signer": {
      "type": "array",
      "items":{
        "type": "object",
        "properties":{
          "id": {
            "type": "string"
          },
          "sig": {
            "type": "string"
          }
        },
        "required":[
          "id",
          "sig"
        ]
      }
    },
    "certificate":{
      "type": "array",
      "itmes": {
            "type": "object",
            "properties": {
              "id": {
                    "type": "string"
              },
              "pk": {
                    "type": "string"
              }
            },
            "required": [
              "id",
              "pk"
            ]
      }
    },
    "producer": {
      "type": "object",
      "properties":{
       "id": {
        "type": "string"
        },
        "sig": {
          "type": "string"
        }
      },
      "required" : [
        "id",
        "sig"
      ]
    }
  },
  "required": [
    "block",
    "tx",
	"aggz",
    "state",
    "signer",
	"certificate",
    "producer"
  ]
})"_json;

const auto SCHEMA_JOIN = R"({
  "title": "Join",
  "type": "object",
  "properties": {
    "time": {
      "type": "string"
    },
    "world": {
      "type": "string"
    },
    "chain": {
      "type": "string"
    },
    "signer": {
      "type": "string"
    },
    "merger": {
      "type": "string"
    }
  },
  "required": [
    "time",
    "world",
    "chain",
    "signer",
    "merger"
  ]
})"_json;
const auto SCHEMA_RESPONSE_FIRST = R"({
  "title": "Response 1 to Challenge",
  "type": "object",
  "properties": {
    "time": {
      "type": "string"
    },
     "sn": {
      "type": "string"
    },
    "dh": {
      "type": "object",
      "properties": {
        "x": {
          "type": "string"
        },
        "y": {
          "type": "string"
        }
      },
      "required": [
        "x",
        "y"
      ]
    },
    "user": {
      "type": "object",
      "properties": {
        "id": {
          "type": "string"
        },
        "pk": {
          "type": "string"
        },
        "sig": {
          "type": "string"
        }
      },
      "required": [
        "id",
        "pk",
        "sig"
      ]
    }
  },
  "required": [
    "time",
    "sn",
    "dh",
    "user"
  ]
})"_json;
const auto SCHEMA_SUCCESS = R"({
  "title": "Success in Key Exchange",
  "type": "object",
  "properties": {
    "time": {
      "type": "string"
    },
    "user": {
      "type": "string"
    },
    "val": {
      "type": "boolean"
    }
  },
  "required": [
    "time",
    "user",
    "val"
  ]
})"_json;
const auto SCHEMA_SSIG = R"({
  "title": "Signer's Signature",
  "type": "object",
  "properties": {
    "block": {
      "type": "object",
      "properties": {
        "id":{
          "type": "string"
        }
      },
      "required":[
        "id"
      ]
    },
    "signer": {
      "type": "object",
      "properties": {
       "id": {
        "type": "string"
        },
        "sig": {
          "type": "string"
        }
      },
      "required" : [
        "id",
        "sig"
      ]
    }
  },
  "required": [
    "block",
    "signer"
  ]
})"_json;
const auto SCHEMA_TX = R"({
  "title": "Transaction",
  "type": "object",
  "properties": {
    "txid": {
      "type": "string"
    },
    "world": {
      "type": "string"
    },
    "chain": {
      "type": "string"
    },
    "time": {
      "type": "string"
    },
    "seed": {
      "type": "string"
    },
    "body": {
      "type": "object",
      "properties": {
        "cid": {
          "type": "string"
        },
        "input": {
          "type": "array",
          "items": {
            "type": "array",
            "itmes": {
              "type": "string"
            }
          }
        }
      },
      "required": [
        "cid",
        "input"
      ]
    },
    "user":{
      "type": "object",
      "properties": {
        "id": {
          "type": "string"
        },
        "pk": {
          "type": "string"
        },
        "sig": {
          "type": "string"
        }
      },
      "required": [
        "id",
        "pk",
        "sig"
      ]
    },
    "endorser": {
      "type": "array",
      "items": {
        "type": "object",
        "properties": {
          "id": {
            "type": "string"
          },
          "pk": {
            "type": "string"
          },
          "sig": {
            "type": "string"
          }
        },
        "required": [
          "id",
          "pk",
          "sig"
        ]
      }
    }
  },
  "required": [
    "txid",
    "world",
    "chain",
    "time",
    "seed",
    "body",
    "user",
    "endorser"
  ]
})"_json;

class JsonValidator {
public:
  static bool validateSchema(nlohmann::json &json_obj, MessageType msg_type) {
    try {
      schema_map[msg_type].validate(json_obj);
      return true;
    } catch (const std::exception &e) {
      return false;
    }
  }

private:
  static SchemaCheckMap initMap() {
    SchemaCheckMap init_map;

    json_validator validator_ping;
    validator_ping.set_root_schema(SCHEMA_PING);
    init_map[MessageType::MSG_PING] = validator_ping;

    json_validator validator_req_block;
    validator_req_block.set_root_schema(SCHEMA_REQ_BLOCK);
    init_map[MessageType::MSG_REQ_BLOCK] = validator_req_block;

    json_validator validator_block;
    validator_block.set_root_schema(SCHEMA_BLOCK);
    init_map[MessageType::MSG_BLOCK] = validator_block;

    json_validator validator_join;
    validator_join.set_root_schema(SCHEMA_JOIN);
    init_map[MessageType::MSG_JOIN] = validator_join;

    json_validator validator_response_first;
    validator_response_first.set_root_schema(SCHEMA_RESPONSE_FIRST);
    init_map[MessageType::MSG_RESPONSE_1] = validator_response_first;

    json_validator validator_success;
    validator_success.set_root_schema(SCHEMA_SUCCESS);
    init_map[MessageType::MSG_SUCCESS] = validator_success;

    json_validator validator_ssig;
    validator_ssig.set_root_schema(SCHEMA_SSIG);
    init_map[MessageType::MSG_SSIG] = validator_ssig;

    json_validator validator_tx;
    validator_tx.set_root_schema(SCHEMA_TX);
    init_map[MessageType::MSG_TX] = validator_tx;

    return init_map;
  }
  inline static SchemaCheckMap schema_map = initMap();
};

} // namespace net_plugin
} // namespace gruut