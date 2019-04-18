class CreateContract < ActiveRecord::Migration[5.2]
  def change
    create_table :contracts, id: false do |t|
      t.integer :cidx, primary_key: true
      t.string :cid, limit: 255, index: { unique: true }
      t.integer :after, limit: 8
      t.integer :before, limit: 8
      t.column :author, 'char(44)'
      t.text :friends
      t.text :contract
      t.string :desc, limit: 500
      t.string :sigma, limit: 100
    end
  end
end
