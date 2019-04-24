class CreateTransaction < ActiveRecord::Migration[5.2]
  def change
    create_table :transactions, id: false do |t|
      t.integer :tsidx, primary_key: true
      t.column :tx_id, 'char(44)', index: { unique: true }
      t.integer :tx_time, limit: 8
      t.string :tx_contract_id, limit: 255
      t.integer :tx_fee_author
      t.integer :tx_fee_user
      t.column :tx_user, 'char(44)'
      t.text :tx_user_pk
      t.column :tx_receiver, 'char(44)'
      t.mediumtext :tx_input
      t.string :tx_user_sig, limit: 100
      t.mediumtext :tx_agg_cbor
      t.column :block_id, 'char(44)'
      t.integer :tx_pos
      t.mediumtext :tx_output
    end
  end
end
