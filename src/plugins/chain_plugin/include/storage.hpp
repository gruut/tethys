#ifndef GRUUT_PUBLIC_MERGER_STORAGE_HPP
#define GRUUT_PUBLIC_MERGER_STORAGE_HPP

#include "leveldb/cache.h"
#include "leveldb/db.h"
#include "leveldb/options.h"
#include "leveldb/write_batch.h"

#include "../../../../lib/json/include/json.hpp"
#include "../../../../lib/gruut-utils/src/bytes_builder.hpp"
#include "../../../../lib/gruut-utils/src/sha256.hpp"
#include "../../../../lib/gruut-utils/src/time_util.hpp"
#include "../config/storage_type.hpp"
#include "static_merkle_tree.hpp"

#include <botan-2/botan/asn1_time.h>
#include <botan-2/botan/auto_rng.h>
#include <botan-2/botan/data_src.h>
#include <botan-2/botan/exceptn.h>
#include <botan-2/botan/pkcs8.h>
#include <botan-2/botan/pubkey.h>
#include <botan-2/botan/rsa.h>
#include <botan-2/botan/x509cert.h>

#include <boost/filesystem/operations.hpp>
#include <cmath>
#include <deque>
#include <fstream> // ifstream 클래스
#include <iostream>
#include <map>

using namespace std;

typedef unsigned int uint;

#define _D_CUR_LAYER 4
enum { BLOCK_ID, USER_ID, VAR_TYPE, VAR_NAME, VAR_VALUE, PATH };
enum { SUCCESS = 0, COIN_VALUE = -1, DATA_DUPLICATE = -2, DATA_NOT_EXIST = -3 };
enum { NO_DATA = -2, DB_DATA = -1, CUR_DATA = _D_CUR_LAYER };

namespace gruut {
//
//class Storage : public TemplateSingleton<Storage> {
//
//private:
//  int MAX_LAYER_SIZE;
//  mariaDb m_server;
//  string m_db_path;
//
//  leveldb::Options m_options;
//  leveldb::WriteOptions m_write_options;
//  leveldb::ReadOptions m_read_options;
//
//  leveldb::DB *m_db_block_header;
//  leveldb::DB *m_db_block_raw;
//  leveldb::DB *m_db_latest_block_header;
//  leveldb::DB *m_db_transaction;
//  leveldb::DB *m_db_blockid_height;
//  leveldb::DB *m_db_ledger;
//  leveldb::DB *m_db_backup;
//
//  leveldb::WriteBatch m_batch_block_header;
//  leveldb::WriteBatch m_batch_block_raw;
//  leveldb::WriteBatch m_batch_latest_block_header;
//  leveldb::WriteBatch m_batch_transaction;
//  leveldb::WriteBatch m_batch_blockid_height;
//  leveldb::WriteBatch m_batch_ledger;
//  leveldb::WriteBatch m_batch_backup;
//};
//
//public:
//Storage();
//~Storage();
//
//void setBlocksByJson();
//
//void parseBlockToLayer(Block block);
//int addCommand(json transaction, Value &val);
//int sendCommand(json transaction);
//int newCommand(json transaction);
//int delCommand(json transaction);
//
//void testStorage();
//void testForward(Block block);
//void testBackward();
//
//bool saveLedger(const std::string &key, const std::string &value);
//std::string readLedgerByKey(const std::string &key);
//void clearLedger();
//void flushLedger();
//bool empty();
//
//void saveBackup(const std::string &key, const std::string &value);
//std::string readBackup(const std::string &key);
//void flushBackup();
//void clearBackup();
//void delBackup(const std::string &block_id_b64);
//
//private:
//void readConfig();
//void setupDB();
//void destroyDB();
} // namespace gruut

#endif // WORKSPACE_STORAGE_HPP
