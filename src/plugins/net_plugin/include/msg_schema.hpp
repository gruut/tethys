#pragma once

#include "../include/json-schema.hpp"
#include "config/message.hpp"
#include <map>
#include <vector>

namespace gruut {
namespace net {

using nlohmann::json;
using nlohmann::json_schema_draft4::json_validator;
using SchemaCheckMap = std::map<MessageType, json_validator>;

const json SCHEMA_REQ_BLOCK = R"({
  "title": "block request",
  "type": "object",
  "properties": {
    "mID": {
      "type": "string"
    },
    "time": {
      "type": "string"
    },
    "mCert": {
      "type": "string"
    },
    "hgt": {
      "type": "string"
    },
    "prevHash": {
      "type": "string"
    },
    "hash": {
      "type": "string"
    },
    "mSig": {
      "type": "string"
    }
  },
  "required": [
    "mID",
    "time",
    "mCert",
    "hgt",
    "mSig"
  ]
})"_json;
const json SCHEMA_BLOCK = R"({
  "title": "Block",
  "type": "object",
  "properties": {
    "mID": {
      "type": "string"
    },
    "blockraw": {
      "type": "string"
    },
    "tx": {
      "type": "array",
      "items": {
        "type": "object",
        "properties": {
          "txid": {
            "type": "string"
          },
          "time": {
            "type": "string"
          },
          "rID": {
            "type": "string"
          },
          "type": {
            "type": "string"
          },
          "content": {
            "type": "array",
            "item": {
              "type": "string"
            }
          },
          "rSig": {
            "type": "string"
          }
        },
        "required": [
          "txid",
          "time",
          "rID",
          "type",
          "content",
          "rSig"
        ]
      }
    }
  },
  "required": [
    "blockraw",
    "tx"
  ]
})"_json;
const json SCHEMA_JOIN = R"({
  "title": "Join",
  "type": "object",
  "properties": {
    "sID": {
      "type": "string"
    },
    "time": {
      "type": "string"
    },
    "ver": {
      "type": "string"
    },
    "cID": {
      "type": "string"
    }
  },
  "required": [
    "sID",
    "time",
    "ver",
    "cID"
  ]
})"_json;
const json SCHEMA_RESPONSE_FIRST = R"({
  "title": "Response 1 to Challenge",
  "type": "object",
  "properties": {
    "sID": {
      "type": "string"
    },
    "time": {
      "type": "string"
    },
     "cert": {
      "type": "string"
    },
    "sN": {
      "type": "string"
    },
    "dhx": {
      "type": "string"
    },
    "dhy": {
      "type": "string"
    },
    "sig": {
      "type": "string"
    }
  },
  "required": [
    "sID",
    "time",
    "cert",
    "sN",
    "dhx",
    "dhy",
    "sig"
  ]
})"_json;
const json SCHEMA_SUCCESS = R"({
  "title": "Success in Key Exchange",
  "type": "object",
  "properties": {
    "sID": {
      "type": "string"
    },
    "time": {
      "type": "string"
    },
     "val": {
      "type": "boolean"
    }
  },
  "required": [
    "sID",
    "time",
    "val"
  ]
})"_json;
const json SCHEMA_SSIG = R"({
  "title": "Signer's Signature",
  "type": "object",
  "properties": {
    "sID": {
      "type": "string"
    },
    "time": {
      "type": "string"
    },
    "sig": {
      "type": "string"
    }
  },
  "required": [
    "sID",
    "time",
    "sig"
  ]
})"_json;
const json SCHEMA_ERROR = R"({
  "title": "Error",
  "type": "object",
  "properties": {
    "sender": {
      "type": "string"
    },
    "time": {
      "type": "string"
    },
    "code": {
      "type": "string"
    },
    "info": {
      "type": "string"
    }
  },
  "required": [
    "sender",
    "time",
    "type"
]
})"_json;
const json SCHEMA_TX = R"({
  "title": "Transaction",
  "type": "object",
  "properties": {
    "txid": {
      "type": "string"
    },
    "time": {
      "type": "string"
    },
    "rID": {
      "type": "string"
    },
    "type": {
      "type": "string"
    },
    "content": {
      "type": "array",
      "item": {
        "type": "string"
      }
    },
    "rSig": {
      "type": "string"
    }
  },
  "required": [
    "txid",
    "time",
    "rID",
    "type",
    "content",
    "rSig"
  ]
})"_json;
const json SCHEMA_REQ_CHECK = R"({
  "title": "Request Check",
  "type": "object",
  "properties": {
    "sender": {
      "type": "string"
    },
    "time": {
      "type": "string"
    },
    "dID": {
      "type": "string"
    },
    "txid": {
      "type": "string"
    }
  },
  "required": [
    "sender",
    "time",
    "dID",
    "txid"
  ]
})"_json;

const json SCHEMA_REQ_HEADER_CHECK = R"({
  "title": "Request Block Header",
  "type": "object",
  "properties": {
    "rID": {
      "description": "requestor's ID",
      "type": "string"
    },
    "time": {
      "description": "요청 시간",
      "type": "string"
    },
    "rCert": {
      "description": "requestor's certificate",
      "type": "string"
    },
    "hgt": {
      "description": "block height",
      "type": "string"
    },
    "rSig": {
      "description": "requestor's signature",
      "type": "string"
    }
  },
  "required": [
    "rID",
    "time",
    "rCert",
    "hgt",
    "rSig"
  ]
})"_json;
const json SCHEMA_REQ_STATUS = R"({
  "title": "Request Status",
  "type": "object",
  "properties": {
    "mID": {
      "type": "string"
    },
    "time": {
      "type": "string"
    }
  },
  "required": [
    "mID",
    "time"
  ]
})"_json;
const json SCHEMA_RES_STATUS = R"({
  "title": "Response Status",
  "type": "object",
  "properties": {
    "mID": {
      "type": "string"
    },
    "time": {
      "type": "string"
    },
    "mCert": {
      "type": "string"
    },
    "hgt": {
      "type": "string"
    },
    "hash": {
      "type": "string"
    },
    "mSig": {
      "type": "string"
    }
  },
  "required": [
    "mID",
    "time",
    "mCert",
    "hgt",
    "hash",
    "mSig"
  ]
})"_json;

class JsonValidator {
public:
  static bool validateSchema(json &json_obj, MessageType msg_type) {
    try {
      schema_map[msg_type].validate(json_obj);
      return true;
    }
    catch(const std::exception &e){
      return false;
    }
  }
  static SchemaCheckMap schema_map;
private:

  static SchemaCheckMap initMap() {
	SchemaCheckMap init_map;

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

	json_validator validator_error;
	validator_error.set_root_schema(SCHEMA_ERROR);
	init_map[MessageType::MSG_ERROR] = validator_error;

	json_validator validator_tx;
	validator_tx.set_root_schema(SCHEMA_TX);
	init_map[MessageType::MSG_TX] = validator_tx;

	json_validator validator_req_check;
	validator_req_check.set_root_schema(SCHEMA_REQ_CHECK);
	init_map[MessageType::MSG_REQ_CHECK] = validator_req_check;

	json_validator validator_req_header;
	validator_req_header.set_root_schema(SCHEMA_REQ_HEADER_CHECK);
	init_map[MessageType::MSG_REQ_HEADER] = validator_req_header;

	json_validator validator_req_status;
	validator_req_status.set_root_schema(SCHEMA_REQ_STATUS);
	init_map[MessageType::MSG_REQ_STATUS] = validator_req_status;

	json_validator validator_res_status;
	validator_res_status.set_root_schema(SCHEMA_RES_STATUS);
	init_map[MessageType::MSG_RES_STATUS] = validator_res_status;

	return init_map;
  }
};

SchemaCheckMap JsonValidator::schema_map = initMap();

}
}