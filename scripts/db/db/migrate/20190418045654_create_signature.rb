class CreateSignature < ActiveRecord::Migration[5.2]
  def change
    create_table :signatures, id: false do |t|
      t.integer :ssidx, primary_key: true
      t.integer :bsidx, index: true, null: false
      t.column :ss_id, 'char(44)'
      t.string :ss_sig, limit: 100
      t.integer :ss_pos
    end
  end
end
