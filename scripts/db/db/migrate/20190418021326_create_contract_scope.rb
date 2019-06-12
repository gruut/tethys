class CreateContractScope < ActiveRecord::Migration[5.2]
  def change
    create_table :contract_scope, id: false do |t|
      t.integer :csidx, primary_key: true
      t.string :contract_id, limit: 255
      t.string :var_name, limit: 255
      t.text :var_value
      t.integer :var_type, limit: 1
      t.text :var_info
      t.integer :up_time, limit: 8
      t.integer :up_block
      t.column :pid, 'char(44)', index: { unique: true }
    end

    add_index :contract_scope, [:contract_id, :var_name], unique: true
  end
end
