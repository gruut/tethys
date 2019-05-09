class CreateEndorser < ActiveRecord::Migration[5.2]
  def change
    create_table :endorsers, id: false do |t|
      t.integer :edidx, primary_key: true
      t.integer :tsidx, index: true, null: false
      t.column :end_id, 'char(44)'
      t.text :end_pk
      t.string :end_sig, limit: 100
    end
  end
end
