#pragma once

#include <eosio/asset.hpp>
#include <eosio/eosio.hpp>
#include <string>
#define hodl_symbol symbol("TEST", 0)
namespace eosiosystem {
   class system_contract;
}

using namespace eosio;

using namespace std;
CONTRACT testtoken : public contract {
   public:
      using contract::contract;
      ACTION create(name issuer, asset maximum_supply, bool transfer_locked);
      ACTION mine(name to, asset quantity, string memo);
      ACTION transfer(name from, name to, asset quantity, string memo);
      ACTION withdraw(name to, asset quantity, string memo);
      ACTION deposit(name hodler, asset quantity, string memo);

   private:
      TABLE account {
         asset    balance;
         uint64_t primary_key()const { return balance.symbol.code().raw(); }
      };
      TABLE currency_stats {
         asset    supply;
         asset    max_supply;
         name     issuer;
         bool  transfer_locked = false;
         uint64_t primary_key()const { return supply.symbol.code().raw(); }
      };
      TABLE balance
      {
         name user;
         eosio::asset funds;
         uint64_t primary_key() const { return funds.symbol.raw(); }
      };
      typedef multi_index<"balance"_n, balance> balance_table;
      typedef eosio::multi_index< "accounts"_n, account > accounts;
      typedef eosio::multi_index< "stat"_n, currency_stats > stats;

      void sub_balance( const name& owner, const asset& value );
      void add_balance( const name& owner, const asset& value, const name& ram_payer );
};


