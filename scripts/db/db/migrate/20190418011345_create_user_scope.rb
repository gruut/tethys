class CreateUserScope < ActiveRecord::Migration[5.2]
  def change
    create_table :user_scope, id: false do |t|
      t.integer :usidx, primary_key: true
      t.string :var_name, limit: 255
      t.text :var_value
      t.integer :var_type, limit: 1
      t.column :var_owner, 'char(44)'
      t.integer :up_time, limit: 8
      t.integer :up_block
      t.mediumtext :tag
      t.column :pid, 'char(44)'
    end
  end
end
