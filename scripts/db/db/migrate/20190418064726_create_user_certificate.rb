class CreateUserCertificate < ActiveRecord::Migration[5.2]
  def change
    create_table :user_certificates, id: false do |t|
      t.integer :ucidx, primary_key: true
      t.column :uid, 'char(44)'
      t.column :sn, 'char(40)', index: { unique: true }
      t.integer :nvbefore, limit: 8
      t.integer :nvafter, limit: 8
      t.mediumtext :x509
    end
  end
end
