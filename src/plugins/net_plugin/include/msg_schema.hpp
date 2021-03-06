#pragma once

#include "../../../../lib/json/include/json-schema.hpp"
#include "../config/include/message.hpp"
#include <map>
#include <vector>

namespace tethys {
namespace net_plugin {

using nlohmann::json_schema_draft4::json_validator;
using SchemaMap = std::map<MessageType, nlohmann::json>;

static SchemaMap schema_map = {{MessageType::MSG_PING,
                                R"({
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
})"_json},
                               {MessageType::MSG_REQ_BLOCK,
                                R"({
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
})"_json},
                               {MessageType::MSG_BLOCK,
                                R"({
  "title": "Block",
  "type": "object",
  "properties": {
    "btime": {
      "type": "string"
    },
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
        "previd":{
          "type": "string"
        },
        "link":{
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
        "previd",
        "link",
        "hash"
      ]
    },
    "tx":{
      "type": "array",
      "items":{
        "type": "string"
      }
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
        "csroot":{
          "type": "string"
        },
        "sgroot":{
          "type": "string"
        }
      },
      "required":[
        "txroot",
        "usroot",
        "csroot",
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
      "items": {
        "type": "object",
        "properties": {
          "id": {
            "type": "string"
          },
          "cert": {
           "type": "array",
           "items": {
             "type": "string"
           }
          }
        },
        "required": [
          "id",
          "cert"
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
    "btime",
    "block",
    "tx",
    "state",
    "signer",
    "certificate",
    "producer"
  ]
})"_json},
                               {MessageType::MSG_REQ_BONE,
                                R"({
  "title": "Request back bone",
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
        "id" : {
            "type": "string"
        },
        "height" : {
            "type": "string"
        }
      },
      "required": [
        "id",
        "height"
      ]
    },
    "merger": {
      "type": "object",
      "properties": {
        "id": {
          "type": "string"
        },
        "sig": {
          "type": "string"
        }
      },
      "required": [
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
})"_json},
                               {MessageType::MSG_BONE,
                                R"({
  "title": "Request back bone",
  "type": "object",
  "properties": {
    "time": {
      "type": "string"
    },
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
        "previd":{
          "type": "string"
        },
        "link":{
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
        "previd",
        "link",
        "hash"
      ]
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
        "csroot":{
          "type": "string"
        },
        "sgroot":{
          "type": "string"
        }
      },
      "required":[
        "txroot",
        "usroot",
        "csroot",
        "sgroot"
      ]
    },
    "count": {
      "type": "object",
      "properties": {
        "tx": {
            "type": "string"
        },
        "signer": {
            "type": "string"
        }
      },
      "required": [
        "tx",
        "signer"
      ]
    },
    "producer": {
      "type": "object",
      "properties": {
        "id": {
          "type": "string"
        },
        "cert": {
         "type": "array",
         "items": {
           "type": "string"
         }
        },
        "sig": {
          "type": "string"
        }
      },
      "required": [
        "id",
        "cert",
        "sig"
      ]
    }
  },
  "required": [
    "time",
    "block",
    "state",
    "count",
    "producer"
  ]
})"_json},
                               {MessageType::MSG_JOIN,
                                R"({
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
    "user": {
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
    "user",
    "merger"
  ]
})"_json},
                               {MessageType::MSG_RESPONSE_1,
                                R"({
  "title": "Response 1 to Challenge",
  "type": "object",
  "properties": {
    "time": {
      "type": "string"
    },
     "un": {
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
    "un",
    "dh",
    "user"
  ]
})"_json},
                               {MessageType::MSG_SUCCESS,
                                R"({
  "title": "Success in Key Exchange",
  "type": "object",
  "properties": {
    "time": {
      "type": "string"
    },
    "user": {
      "type": "string"
    },
    "mode": {
      "type": "string"
    },
    "val": {
      "type": "boolean"
    }
  },
  "required": [
    "time",
    "user",
    "mode",
    "val"
  ]
})"_json},
                               {MessageType::MSG_SSIG,
                                R"({
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
})"_json},
                               {MessageType::MSG_TX,
                                R"({
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
        "receiver": {
          "type": "string"
        },
        "fee": {
          "type": "string"
        },
        "input": {
          "type": "array",
          "items": {
            "type": "array",
            "items": {
                "type": "object"
              }
            }
          }
        }
      },
      "required": [
        "cid",
        "receiver",
        "fee",
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
    },
  "required": [
    "txid",
    "world",
    "chain",
    "time",
    "body",
    "user",
    "endorser"
  ]
})"_json},
                               {MessageType::MSG_SETUP_MERGER,
                                R"({
  "title": "Setup merger",
  "type": "object",
  "properties": {
          "enc_sk": { "type": "string" },
          "cert": { "type": "string" }
  },
  "required": [
          "enc_sk",
          "cert"
  ]
})"_json},
                               {MessageType::MSG_REQ_TX_CHECK,
                                R"({
  "title": "Request transaction information",
  "type": "object",
  "properties": {
          "time": { "type": "string" },
          "user": { "type": "string" },
          "world": { "type": "string"},
          "chain": { "type": "string"},
          "txid": { "type": "string"}
  },
  "required": [
          "time",
          "user",
          "world",
          "chain",
          "txid"
  ]
})"_json},
                               {MessageType::MSG_RES_TX_CHECK,
                                R"({
    "title": "Result of REQ_TX_CHECK",
    "type": "object",
    "properties": {
        "time": { "type": "string" },
        "block": { "type": "string"},
        "confirm": { "type": "boolean"},
        "proof": {"type": "string"},
        "output": {"type": "string"},
        "merger": {
            "type": "object",
            "properties":{
                "id": {"type": "string"},
                "sig": {"type": "string"}
            },
            "required":[
                "id",
                "sig"
            ]
        }
    },
    "required": [
        "time",
        "block",
        "confirm",
        "proof",
        "output",
        "merger"
    ]
})"_json},
                               {MessageType::MSG_QUERY,
                                R"({
   "title": "Request Queries",
       "type": "object",
       "properties": {
          "type": { "type": "string" },
          "where": { "type": "object"}
   },
   "required": [
       "type",
       "where"
   ]
})"_json},
                               {MessageType::MSG_RESULT, R"({
    "title": "Result Queries",
    "type": "object",
    "properties": {
        "name": {
            "type": "array",
            "items": {
                "type": "string"
            }
        },
        "data": {
            "type": "array",
            "items": {
                "type": "array",
                "items":{
                    "type": "string"
                }
            }
        }
    },
    "required": [
        "name",
        "data"
    ]
})"_json}};

class JsonValidator {
public:
  static bool validateSchema(nlohmann::json &json_obj, MessageType msg_type) {
    try {
      json_validator validator;
      validator.set_root_schema(schema_map[msg_type]);
      validator.validate(json_obj);
      return true;
    } catch (const std::exception &e) {
      logger::ERROR("[Message Type: {}] message validation error: {}", (uint8_t)msg_type, e.what());
      return false;
    }
  }
};

} // namespace net_plugin
} // namespace tethys