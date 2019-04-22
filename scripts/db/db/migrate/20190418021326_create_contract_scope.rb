class CreateContractScope < ActiveRecord::Migration[5.2]
  def change
    create_table :contract_scope, id: false do |t|
      t.integer :csidx, primary_key: true
      t.string :contract_id, limit: 255, index: { unique: true }
      t.string :var_name, limit: 255
      t.text :var_value
      t.integer :var_type, limit: 1
      t.text :var_info
      t.integer :up_time, limit: 8
      t.column :pid, 'char(43)', index: { unique: true }
    end
  end
end
