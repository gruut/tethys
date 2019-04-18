class CreateUserAttribute < ActiveRecord::Migration[5.2]
  def change
    create_table :user_attributes, id: false do |t|
      t.integer :uaidx, primary_key: true
      t.column :uid, 'char(44)', index: { unique: true }
      t.integer :register_day, limit: 8
      t.integer :gender, limit: 1
      t.string :isc_type, limit: 10
      t.string :isc_code, limit: 10
      t.text :location
      t.integer :age_limit
      t.string :sigma, limit: 100
    end
  end
end
