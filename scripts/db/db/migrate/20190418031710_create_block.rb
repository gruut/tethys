class CreateBlock < ActiveRecord::Migration[5.2]
  def change
    create_table :blocks, id: false do |t|
      t.integer :bsidx, primary_key: true
      t.column :block_id, 'char(44)', index: { unique: true }
      t.integer :block_height
      t.column :block_hash, 'char(44)'
      t.integer :block_time, limit: 8
      t.integer :block_pub_time, limit: 8
      t.column :block_prev_id, 'char(44)'
      t.column :producer_id, 'char(44)'
      t.string :producer_sig, limit: 100
      t.mediumtext :txs
      t.column :tx_root, 'char(44)'
      t.column :us_state_root, 'char(44)'
      t.column :cs_state_root, 'char(44)'
      t.column :sg_root, 'char(44)'
      t.string :aggz, limit: 50
      t.mediumtext :certificate
    end
  end
end
