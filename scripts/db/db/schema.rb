# This file is auto-generated from the current state of the database. Instead
# of editing this file, please use the migrations feature of Active Record to
# incrementally modify your database, and then regenerate this schema definition.
#
# Note that this schema.rb definition is the authoritative source for your
# database schema. If you need to create the application database on another
# system, you should be using db:schema:load, not running all the migrations
# from scratch. The latter is a flawed and unsustainable approach (the more migrations
# you'll amass, the slower it'll run and the greater likelihood for issues).
#
# It's strongly recommended that you check this file into your version control system.

ActiveRecord::Schema.define(version: 2019_04_18_064726) do

  create_table "blocks", primary_key: "bsidx", id: :integer, options: "ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_0900_ai_ci", force: :cascade do |t|
    t.string "block_id", limit: 44
    t.integer "block_height"
    t.string "block_hash", limit: 44
    t.bigint "block_time"
    t.bigint "block_pub_time"
    t.string "block_prev_id", limit: 44
    t.string "producer_id", limit: 44
    t.string "producer_sig", limit: 100
    t.text "txs", limit: 16777215
    t.string "tx_root", limit: 44
    t.string "us_state_root", limit: 44
    t.string "sc_state_root", limit: 44
    t.string "sg_root", limit: 44
    t.string "aggz", limit: 50
    t.text "certificate", limit: 16777215
    t.index ["block_id"], name: "index_blocks_on_block_id", unique: true
  end

  create_table "contract_scope", primary_key: "csidx", id: :integer, options: "ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_0900_ai_ci", force: :cascade do |t|
    t.string "contract_id"
    t.string "var_name"
    t.integer "var_value"
    t.string "var_type", limit: 10
    t.text "var_info"
    t.bigint "up_time"
    t.string "pid", limit: 43
    t.index ["contract_id"], name: "index_contract_scope_on_contract_id", unique: true
    t.index ["pid"], name: "index_contract_scope_on_pid", unique: true
  end

  create_table "endorsers", primary_key: "edidx", id: :integer, options: "ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_0900_ai_ci", force: :cascade do |t|
    t.integer "tsids", null: false
    t.string "end_id", limit: 44
    t.text "end_pk"
    t.string "end_sig", limit: 100
    t.index ["tsids"], name: "index_endorsers_on_tsids"
  end

  create_table "signatures", primary_key: "ssidx", id: :integer, options: "ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_0900_ai_ci", force: :cascade do |t|
    t.integer "bsidx", null: false
    t.string "ss_id", limit: 44
    t.string "ss_sig", limit: 100
    t.integer "ss_pos"
    t.index ["bsidx"], name: "index_signatures_on_bsidx"
  end

  create_table "transactions", primary_key: "tsidx", id: :integer, options: "ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_0900_ai_ci", force: :cascade do |t|
    t.string "tx_id", limit: 44
    t.bigint "tx_time"
    t.string "tx_seed", limit: 22
    t.string "tx_contract_id"
    t.integer "tx_fee_author"
    t.integer "tx_fee_user"
    t.string "tx_user", limit: 44
    t.text "tx_user_pk"
    t.string "tx_receiver", limit: 44
    t.text "tx_input", limit: 16777215
    t.string "tx_user_sig", limit: 100
    t.text "tx_agg_cbor", limit: 16777215
    t.string "block_id", limit: 44
    t.integer "tx_pos"
    t.text "tx_output", limit: 16777215
    t.index ["tx_id"], name: "index_transactions_on_tx_id", unique: true
  end

  create_table "user_certificates", primary_key: "ucidx", id: :integer, options: "ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_0900_ai_ci", force: :cascade do |t|
    t.string "uid", limit: 44
    t.string "sn", limit: 40
    t.bigint "nvbefore"
    t.bigint "nvafter"
    t.text "x509", limit: 16777215
    t.index ["sn"], name: "index_user_certificates_on_sn", unique: true
  end

  create_table "users", primary_key: "usidx", id: :integer, options: "ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_0900_ai_ci", force: :cascade do |t|
    t.string "var_name"
    t.integer "var_value"
    t.string "var_type", limit: 10
    t.string "var_owner", limit: 44
    t.bigint "up_time"
    t.integer "up_block"
    t.text "condition", limit: 16777215
    t.string "pid", limit: 43
  end

end