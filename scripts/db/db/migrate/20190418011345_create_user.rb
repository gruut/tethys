class CreateUser < ActiveRecord::Migration[5.2]
  def change
    create_table :users, id: false do |t|
      t.integer :usidx, primary_key: true
      t.string :var_name, limit: 255
      t.integer :var_value
      t.string :var_type, limit: 10
      t.column :var_owner, 'char(44)'
      t.integer :up_time, limit: 8
      t.integer :up_block
      t.mediumtext :condition
      t.column :pid, 'char(43)'
    end
  end
end
