#include <testtoken.hpp>

ACTION testtoken :: deposit(name hodler, asset quantity, string memo)
{
   if (hodler == get_self())
   {
      print("These are not the droids you are looking for.");
      return;
   }

   check(quantity.amount == 300, "You must deposit 300 tokens");
   check(quantity.symbol == hodl_symbol, "These are not the droids you are looking for.");
   transfer(hodler, get_self(), quantity, "Depsit 300 tokens to Contract address"); 
   balance_table balance(get_self(), hodler.value);
   auto hodl_it = balance.find(hodl_symbol.raw());
   auto itr = balance.find(get_self().value);

   if (itr != balance.end())
      balance.modify(hodl_it, get_self(), [&](auto &row) {
         row.user = hodler;
         row.funds += quantity;
      });
   else
      balance.emplace(get_self(), [&](auto &row) {
         row.user = hodler;
         row.funds = quantity;
   });
}
ACTION testtoken :: withdraw(name to, asset quantity, string memo)
{
   if (to == get_self())
   {
      print("These are not the droids you are looking for.");
      return;
   }

   check(quantity.amount != 400, "Error");
   check(quantity.amount == 200, "You must withdraw 200 tokens");
   check(quantity.symbol == hodl_symbol, "These are not the droids you are looking for.");
   transfer(get_self(),to , quantity, "Withdraw 200 tokens to user"); 
   balance_table balance(get_self(), to.value);
   auto hodl_it = balance.find(hodl_symbol.raw());
   auto itr = balance.find(to.value);

   if (itr != balance.end()) {
      balance.modify(hodl_it, get_self(), [&](auto &row) {
         row.user = to;
         row.funds -= quantity;
      });
   }
   
}

ACTION testtoken::create() {
     require_auth(get_self());

     auto sym = symbol("TEST", 0); 
     auto maximum_supply = asset(5000, sym);

     stats statstable(get_self(), sym.code().raw());
     auto existing = statstable.find(sym.code().raw());
     check(existing == statstable.end(), "token with symbol already created");

     statstable.emplace(get_self(), [&](auto &s) {
        s.supply.symbol = sym;
        s.max_supply = maximum_supply;
        s.issuer = get_self();
     });
}


ACTION testtoken::mine(const asset &quantity, const string &memo) {
     require_auth(get_self());
   //   asset &quantity = asset(100.0000, "TEST"); 
     auto sym = quantity.symbol;
     auto NEWTESTsym_code = symbol("TEST", 0);
     check(sym.code() == NEWTESTsym_code.code(), "This contract can handle NEWTEST tokens only.");
     check(sym.is_valid(), "invalid symbol name");
     check(memo.size() <= 256, "memo has more than 256 bytes");

     stats statstable(get_self(), sym.code().raw());
     auto existing = statstable.find(sym.code().raw());
     check(existing != statstable.end(), "token with symbol does not exist, create token before issue");

     const auto& existing_token = *existing;
     require_auth( existing_token.issuer );

     check(quantity.is_valid(), "invalid quantity");
     check(quantity.amount == 100, "Must be 100 tokens");
     check(quantity.symbol == existing_token.supply.symbol, "symbol precision mismatch");
     statstable.modify(existing_token, same_payer, [&](auto &s) {
        s.supply += quantity;
     });

     add_balance(existing_token.issuer, quantity, existing_token.issuer);
  }

ACTION testtoken::transfer(const name &from,
                       const name &to,
                       const asset &quantity,
                       const string &memo) {
   check(from != to, "cannot transfer to self");
   require_auth(from);
   check(is_account(to), "to account does not exist");
   auto sym = quantity.symbol.code();

   auto NEWTESTsym_code = symbol("TEST", 0); 
   check(sym == NEWTESTsym_code.code(), "This contract can handle NEWTEST tokens only.");
   stats statstable(get_self(), sym.raw());
   const auto &st = statstable.get(sym.raw());

   require_recipient(from);
   require_recipient(to);

   check(quantity.is_valid(), "invalid quantity");
   check(quantity.amount > 0, "must transfer positive quantity");
   check(quantity.symbol == st.supply.symbol, "symbol precision mismatch");
   check(memo.size() <= 256, "memo has more than 256 bytes");

   auto payer = has_auth(to) ? to : from;

   sub_balance(from, quantity);
   add_balance(to, quantity, payer);
}

void testtoken::sub_balance( const name& owner, const asset& value ) {
   accounts from_acnts( get_self(), owner.value );

   const auto& from = from_acnts.get( value.symbol.code().raw(), "no balance object found" );
   check( from.balance.amount >= value.amount, "overdrawn balance" );

   from_acnts.modify( from, owner, [&]( auto& a ) {
         a.balance -= value;
      });
}

void testtoken::add_balance( const name& owner, const asset& value, const name& ram_payer )
{
   accounts to_acnts( get_self(), owner.value );
   auto to = to_acnts.find( value.symbol.code().raw() );
   if( to == to_acnts.end() ) {
      to_acnts.emplace( ram_payer, [&]( auto& a ){
        a.balance = value;
      });
   } else {
      to_acnts.modify( to, same_payer, [&]( auto& a ) {
        a.balance += value;
      });
   }
}



