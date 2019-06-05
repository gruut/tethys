/*
 * Copyright 2018, 2019 DaeHun Nyang, DaeHong Min
 * Free for non-commercial and education use.
 * For commercial use, contact nyang@inha.ac.kr
 */

#ifndef GRUUTSCE_RCC_QP_HPP
#define GRUUTSCE_RCC_QP_HPP

#define USE_BOTAN_HASH 1

#include <stdint.h>
#include <stdlib.h>
#include <inttypes.h>
#include <stdio.h>
#include <limits.h>
#include <string.h>
#include <time.h>
#include <stdbool.h>
#include <math.h>
#include <memory>
#include <vector>
#include <algorithm>
#include <random>
#include <cstring>

#ifdef USE_BOTAN_HASH

#include <botan-2/botan/hash.h>

#else
#endif

constexpr int HASH_TABLE_SIZE = 1048576; // 0x100000;
constexpr int CACHE_SIZE = 131072; // ‬(1 << 17);
constexpr int JUMP_THRESHOLD = 100;  // hashtable jump JUMP_THRESHOLD
constexpr int VIRTUAL_VECTOER_SIZE = 8;
constexpr int LC_COUNTER_SINGLEFR_IDXSIZE = 786432; // 3*1024*1024/4;			// single layer Flow Regulator를 사용할 때 변수
constexpr int LC_COUNTER_DOUBLEFRE_IDXSIZE = 4718592; // 3*1024*1024/4*6;		// double layer Flow Regulator를 사용할 때 변수

struct input_array_entry {
  uint64_t hv;
  std::shared_ptr<input_array_entry> next;

  input_array_entry() : hv(0) {}
};

struct lru_entry {
  uint64_t hash_value;
  std::shared_ptr<lru_entry> prev;
  std::shared_ptr<lru_entry> next;

  lru_entry() : hash_value(0) {}
};

struct entry_t {
  uint64_t hash_value;
  uint64_t timestamp;
  float total_counter;
  std::string data;

  entry_t() : hash_value(0), timestamp(0), total_counter(0.0) {}
};

struct hashtable_t {
  uint32_t usage;
  uint32_t size;
  uint64_t insert;
  uint64_t total_jump;
  uint64_t eviction;
  std::vector<entry_t> table;

  hashtable_t() : usage(0), size(0), insert(0), total_jump(0), eviction(0) {}
};

struct rcc32_t {
  uint32_t memory_fr_limit;
  std::vector<uint32_t> rcounter;
  hashtable_t htable;

  rcc32_t() : memory_fr_limit(0) {}
};


struct chain_entry {
  uint64_t hv;
  std::shared_ptr<chain_entry> next;

  chain_entry() : hv(0) {}
};

struct chain_hash_table {
  uint64_t usage;
  uint64_t insert;
  uint64_t evict;
  std::vector<std::shared_ptr<chain_entry>> htable;

  chain_hash_table() : usage(0), insert(0), evict(0) {}
};

class RCCQP {
private:
  std::shared_ptr<chain_hash_table> hash_table;
  std::shared_ptr<rcc32_t> rcc;
  bool rcc_qp_flag;
  std::shared_ptr<lru_entry> lru_head;
  std::shared_ptr<lru_entry> lru_tail;
  std::shared_ptr<input_array_entry> input_array_head;
  std::shared_ptr<input_array_entry> input_array_tail;
  std::mt19937 prng;

public:
  RCCQP() : rcc_qp_flag(false) {
    std::random_device rand_device;
    prng.seed(rand_device());
  }

  ~RCCQP() {
    chainClear();
  }

  int test() {
    uint64_t hv;

    reset();

    rcc_qp_flag = false;
    lruInit();
    rccCreate(1);

    scanf("%" PRIu64, &hv);
    while (hv != 0) {
      inputArrayInsert(hv);
      hv = 0;
      scanf("%" PRIu64, &hv);
    }

    hv = inputArrayDelete();
    while (hv != 0) {
      if (chainSearch(hv)) {
        hv = inputArrayDelete();

        if (rcc_qp_flag)
          rcc_qp_flag = false;
        continue;
      }

      if (rcc_qp_flag) {
        chainInsert(hv);
        rcc_qp_flag = false;
      }

      hv = inputArrayDelete();
    }

    return 0;
  }

private:

  void reset() {
    inputArrayInit();
    chainInit();
  }

  void inputArrayInit() {
    input_array_head = nullptr;
    input_array_tail = nullptr;
  }

  void chainClear() {
    if (hash_table != nullptr) {
      for (int i = 0; i < hash_table->htable.size(); ++i) {
        hash_table->htable[i] = nullptr;
      }
    }
  }

  void inputArrayInsert(uint64_t hv) {
    std::shared_ptr<input_array_entry> new_entry(new input_array_entry);
    new_entry->hv = hv;
    new_entry->next = nullptr;

    if (input_array_head == nullptr) {
      input_array_head = new_entry;
      input_array_tail = new_entry;
    } else {
      input_array_tail->next = new_entry;
      input_array_tail = new_entry;
    }
  }

  uint64_t inputArrayDelete() {
    if (input_array_head == nullptr)
      return 0;

    uint64_t ret_val = input_array_head->hv;

    if (input_array_head->next == nullptr)
      input_array_head = nullptr;
    else
      input_array_head = input_array_head->next;

    return ret_val;
  }

  void chainInit() {

    chainClear();

    hash_table.reset(new chain_hash_table);
    hash_table->htable.resize(HASH_TABLE_SIZE);

    for (uint32_t i = 0; i < HASH_TABLE_SIZE; ++i)
      hash_table->htable[i] = nullptr;

    hash_table->evict = 0;
    hash_table->insert = 0;
    hash_table->usage = 0;
  }

  bool chainSearch(uint64_t hv) {
    lruUpdate(hv);
    singleFREncode(hv);
    uint32_t idx = hv % HASH_TABLE_SIZE;

    std::shared_ptr<chain_entry> ptr = hash_table->htable[idx];

    if (ptr == nullptr)
      return false;

    if (ptr->hv == hv)
      return true;


    while (ptr->next != nullptr) {
      ptr = ptr->next;

      if (ptr->hv == hv)
        return true;
    }
    return false;
  }

  void chainInsert(uint64_t hv) {
    if (hash_table->usage < CACHE_SIZE) {
      ++hash_table->usage;
    } else {
      uint64_t evict_hv = 0;
      evict_hv = lruEvict();
      chainDelete(evict_hv);
    }

    uint32_t idx = hv % HASH_TABLE_SIZE;
    std::shared_ptr<chain_entry> temp(new chain_entry);
    temp->hv = hv;
    temp->next = hash_table->htable[idx];
    hash_table->htable[idx] = temp;

    ++hash_table->insert;

    lruInsert(hv);
  }

  void chainDelete(uint64_t hv) {
    uint32_t idx = hv % HASH_TABLE_SIZE;
    std::shared_ptr<chain_entry> temp, ptr;

    ptr = hash_table->htable[idx];
    if (ptr == nullptr)
      return;

    if (ptr->hv == hv) {
      if (ptr->next != nullptr)
        hash_table->htable[idx] = ptr->next;
      else
        hash_table->htable[idx] = nullptr;

      ++(hash_table->evict);
      return;
    }

    while (ptr->next != nullptr) {
      if (ptr->next->hv == hv) {
        temp = ptr->next;
        ptr->next = temp->next;
        ++(hash_table->evict);
        return;
      }

      ptr = ptr->next;
    }
  }


  void lruInit() {
    lru_head = nullptr;
    lru_tail = nullptr;
  }

  void lruInsert(uint64_t hv) {
    std::shared_ptr<lru_entry> new_entry(new lru_entry);
    new_entry->hash_value = hv;

    if (lru_head == nullptr)
      lru_tail = new_entry;
    else
      lru_head->prev = new_entry;

    new_entry->next = lru_head;
    lru_head = new_entry;
  }

  uint64_t lruEvict() {
    std::shared_ptr<lru_entry> tmp = lru_tail;

    if (lru_head == nullptr)
      return 0;

    if (lru_head->next == nullptr)
      lru_head = nullptr;
    else
      lru_tail->prev->next = nullptr;

    lru_tail = lru_tail->prev;

    return tmp->hash_value;
  }

  void lruUpdate(uint64_t hv) {

    if (lru_head == nullptr)
      return;

    std::shared_ptr<lru_entry> current = lru_head;
    while (current->hash_value != hv) {
      if (current->next == nullptr)
        return;
      else
        current = current->next;
    }

    if (current == lru_head)
      lru_head = lru_head->next;
    else
      current->prev->next = current->next;

    if (current == lru_tail)
      lru_tail = current->prev;
    else
      current->next->prev = current->prev;

    lruInsert(hv);
  }


  void rccCreate(int layer) {
    rcc.reset(new rcc32_t);

    if (layer == 1)
      rcc->rcounter.resize(LC_COUNTER_SINGLEFR_IDXSIZE);
    else if (layer == 2)
      rcc->rcounter.resize(LC_COUNTER_DOUBLEFRE_IDXSIZE);

    std::fill(rcc->rcounter.begin(), rcc->rcounter.end(), 0);

    rcc->htable.table.reserve(HASH_TABLE_SIZE);
    for (int i = 0; i < HASH_TABLE_SIZE; ++i) {
      rcc->htable.table[i].hash_value = 0;
      rcc->htable.table[i].total_counter = 0;
    }
    rcc->htable.size = HASH_TABLE_SIZE;
    rcc->htable.usage = 0;
    rcc->htable.insert = 0;
    rcc->htable.eviction = 0;
    rcc->htable.total_jump = 0;

    if (layer == 1)
      rcc->memory_fr_limit = LC_COUNTER_SINGLEFR_IDXSIZE;
    else if (layer == 2)
      rcc->memory_fr_limit = LC_COUNTER_DOUBLEFRE_IDXSIZE / 6;

    //rcc->memory_usage = 0;
    //rcc->vector_size = VIRTUAL_VECTOER_SIZE;
  }

  inline uint32_t getMz(uint32_t word, uint32_t vector) {
    float i = (32 - VIRTUAL_VECTOER_SIZE) - (getNumbSetBits(word) - getNumbSetBits(vector & word));
    return (uint32_t) (i / ((32 / VIRTUAL_VECTOER_SIZE) - 1));
  }

  float getKhat(float zeros, float mzeros) {
    float Vs = (zeros / (VIRTUAL_VECTOER_SIZE - 1));
    float Vm = ((mzeros) / (VIRTUAL_VECTOER_SIZE));
    float cs = log(1.0 - 1.0 / (VIRTUAL_VECTOER_SIZE));        // constant for tt
    float k2 = -(VIRTUAL_VECTOER_SIZE - 1) * log(Vs);
    float k1 = log(Vm) / cs;
    float khat = k2 - k1;
    khat = (khat < 0) ? 0 : (khat);
    return khat;
  }

  inline int getBitmaskDIndex(int vector, int D) {
    if (D < 0)
      return 0;

    while (D-- > 0)
      vector &= vector - 1;

    return vector & -vector;    // a word that contains the D's bit in the virtual vector
  }

  inline int getNumbSetBits(uint32_t i) {
    i = i - ((i >> 1) & 0x55555555);
    i = (i & 0x33333333) + ((i >> 2) & 0x33333333);
    return (((i + (i >> 4)) & 0x0F0F0F0F) * 0x01010101) >> 24;
  }

/* Insert a hash_value-value pair into a hash table. */

  void htSet4(uint64_t hash_value, float est) {
    ++(rcc->htable.insert);
    uint32_t loc = 0;
    uint32_t min = -1;
    int expired_loc = -1;
    int hsize = rcc->htable.size;

    for (int64_t qp = 0; qp < JUMP_THRESHOLD; ++qp) {
      // prepare set to another place
      loc = (hash_value + (qp + qp * qp) / 2) % hsize; // hash_value + 0.5i+ 0.5i^2
      entry_t &me = rcc->htable.table[loc];
      if (me.hash_value == 0) {
        me.hash_value = hash_value;
        me.total_counter = est;

        rcc->htable.total_jump += (qp + 1);
        ++(rcc->htable.usage);
        return;
      } else if (me.hash_value == hash_value) {
        me.total_counter += est;

        rcc->htable.total_jump += (qp + 1);
        return;
      }
        // eviction target search
      else if ((me.total_counter) < min) {
        min = me.total_counter;
        expired_loc = loc;
      }
    }

    rcc->htable.total_jump += JUMP_THRESHOLD;
    rcc->htable.eviction++;
    rcc->htable.table[expired_loc].hash_value = hash_value;  // expired
    rcc->htable.table[expired_loc].total_counter = est;  // expired
  }

/* single layer Flow Regulator에 입력 */
  void singleFREncode(uint64_t hv) {
    int left = -1;
    int right = 59;
    uint32_t last_vector = 0;
    uint32_t vector = 0;
    uint64_t rehash = hv;
    int i = 0;

    uint64_t temp;
    while (i < VIRTUAL_VECTOER_SIZE) {
      ++left;
      temp = rehash;
      temp = (temp << left);
      temp = (temp >> right);
      vector |= (0x1 << temp);
      if (last_vector != vector) {
        i++;
        last_vector = vector;
      }
      if (left == 59) {
        rehash = hash(rehash);
        left = -1;
      }
    }

    uint32_t A_index = (hv % (rcc->memory_fr_limit));

    std::uniform_int_distribution<> rand_dist(0, VIRTUAL_VECTOER_SIZE - 1);

    //set random bit
    i = rand_dist(prng);
    uint32_t composed_word = rcc->rcounter[A_index] | getBitmaskDIndex(vector, i);
    uint32_t zeros = VIRTUAL_VECTOER_SIZE - getNumbSetBits(vector & composed_word);

    if (zeros < ((double) VIRTUAL_VECTOER_SIZE * 0.3)) {
      uint32_t mzeros = getMz(composed_word, vector);
      zeros = 2;
      if (mzeros <= 3)
        mzeros = VIRTUAL_VECTOER_SIZE;

      uint32_t bit_mask, test;

      while (zeros < mzeros) {
        i = rand_dist(prng);
        bit_mask = getBitmaskDIndex(vector, i);
        test = bit_mask & composed_word;
        if (test == bit_mask) {
          composed_word ^= bit_mask;
          ++zeros;
        }
      }

      rcc->rcounter[A_index] = composed_word;
      float est = getKhat(2, mzeros) + 1;

      rcc_qp_flag = true;
      htSet4(hv, est);
    }
    rcc->rcounter[A_index] = composed_word;
  }

  uint64_t hash(uint64_t msg_int) {

    std::vector<uint8_t> msg_bytes;
    auto input_size = sizeof(msg_int);

    msg_bytes.reserve(input_size);

    for (auto i = 0; i < input_size; ++i) {
      msg_bytes.push_back(static_cast<uint8_t>(msg_int & 0xFF));
      msg_int >>= 8;
    }

    uint64_t ret_val;

#ifdef USE_BOTAN_HASH
    unique_ptr <Botan::HashFunction> hash_function(Botan::HashFunction::create("SHA-256"));
    hash_function->update(msg_bytes);

    std::vector<uint8_t> hash_result = hash_function->final_stdvec();
    std::memcpy(&ret_val, hash_result.data(), sizeof(uint64_t));
#else

#endif

    return ret_val;
  }
};


#endif //GRUUTSCE_RCC_QP_HPP
