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
      ACTION create();
      ACTION mine(const asset &quantity, const string& memo );
      ACTION transfer( const name&    from,
                     const name&    to,
                     const asset&   quantity,
                     const string&  memo );
      ACTION withdraw(name to, asset quantity, string memo);
      ACTION deposit(name hodler, asset quantity, string memo);

      static asset get_supply( const name& token_contract_account, const symbol_code& sym_code )
      {
         stats statstable( token_contract_account, sym_code.raw() );
         const auto& st = statstable.get( sym_code.raw(), "invalid supply symbol code" );
         return st.supply;
      }

      static asset get_balance( const name& token_contract_account, const name& owner, const symbol_code& sym_code )
      {
         accounts accountstable( token_contract_account, owner.value );
         const auto& ac = accountstable.get( sym_code.raw(), "no balance with specified symbol" );
         return ac.balance;
      }

      // using create_action = eosio::action_wrapper<"create"_n, &token::create>;
      // using issue_action = eosio::action_wrapper<"issue"_n, &token::issue>;
      // using retire_action = eosio::action_wrapper<"retire"_n, &token::retire>;
      // using transfer_action = eosio::action_wrapper<"transfer"_n, &token::transfer>;
      // using open_action = eosio::action_wrapper<"open"_n, &token::open>;
      // using close_action = eosio::action_wrapper<"close"_n, &token::close>;
   private:
      struct [[eosio::table]] account {
         asset    balance;

         uint64_t primary_key()const { return balance.symbol.code().raw(); }
      };

      struct [[eosio::table]] currency_stats {
         asset    supply;
         asset    max_supply;
         name     issuer;

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


