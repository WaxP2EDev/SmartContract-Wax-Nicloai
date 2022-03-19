#include <testtoken.hpp>

ACTION testtoken :: deposit(name hodler, asset quantity, string memo)
{
   if (hodler == get_self())
   {
      print("These are not the droids you are looking for.");
      return;
   }

   check(quantity.amount > 0, "You must deposit at leat 1 tokens");
   check(quantity.symbol == hodl_symbol, "These are not the droids you are looking for.");
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
   check(quantity.amount > 0, "Error, you must withdraw at least 1 tokens");
   check(quantity.symbol == hodl_symbol, "These are not the droids you are looking for.");
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
ACTION testtoken::create(name issuer, asset maximum_supply, bool transfer_locked) {

   require_auth(get_self());

   auto sym = maximum_supply.symbol;
   check(sym.is_valid(), "ERR::CREATE_INVALID_SYMBOL::invalid symbol name");
   check(maximum_supply.is_valid(), "ERR::CREATE_INVALID_SUPPLY::invalid supply");
   check(maximum_supply.amount > 0, "ERR::CREATE_MAX_SUPPLY_MUST_BE_POSITIVE::max-supply must be positive");

   stats statstable(_self, sym.code().raw());
   auto  existing = statstable.find(sym.code().raw());
   check(existing == statstable.end(), "ERR::CREATE_EXISITNG_SYMBOL::token with symbol already exists");

   statstable.emplace(_self, [&](auto &s) {
      s.supply.symbol   = maximum_supply.symbol;
      s.max_supply      = maximum_supply;
      s.issuer          = issuer;
      s.transfer_locked = transfer_locked;
   });
}


void testtoken::mine(name to, asset quantity, string memo) {
   auto sym = quantity.symbol;
   check(sym.is_valid(), "ERR::ISSUE_INVALID_SYMBOL::invalid symbol name");
   auto  sym_name = sym.code().raw();
   stats statstable(_self, sym_name);
   auto  existing = statstable.find(sym_name);
   check(existing != statstable.end(),
      "ERR::ISSUE_NON_EXISTING_SYMBOL::token with symbol does not exist, create token before issue");
   const auto &st = *existing;

   require_auth(st.issuer);
   check(quantity.is_valid(), "ERR::ISSUE_INVALID_QUANTITY::invalid quantity");
   check(quantity.amount > 0, "ERR::ISSUE_NON_POSITIVE::must issue positive quantity");

   check(quantity.symbol == st.supply.symbol, "ERR::ISSUE_INVALID_PRECISION::symbol precision mismatch");
   check(quantity.amount <= st.max_supply.amount - st.supply.amount,
      "ERR::ISSUE_QTY_EXCEED_SUPPLY::quantity exceeds available supply");

   statstable.modify(st, same_payer, [&](auto &s) {
      s.supply += quantity;
   });

   add_balance(st.issuer, quantity, st.issuer);

   if (to != st.issuer) {
      SEND_INLINE_ACTION(*this, transfer, {st.issuer, "active"_n}, {st.issuer, to, quantity, memo});
   }
}

ACTION testtoken::transfer(name from, name to, asset quantity, string memo) {
   check(from != to, "ERR::TRANSFER_TO_SELF::cannot transfer to self");
   require_auth(from);
   check(is_account(to), "ERR::TRANSFER_NONEXISTING_DESTN::to account does not exist");

   auto        sym = quantity.symbol.code();
   stats       statstable(_self, sym.raw());
   const auto &st = statstable.get(sym.raw());

   if (st.transfer_locked) {
      check(has_auth(st.issuer), "Transfer is locked, need issuer permission");
   }

   require_recipient(from, to);

   check(quantity.is_valid(), "ERR::TRANSFER_INVALID_QTY::invalid quantity");
   check(quantity.amount > 0, "ERR::TRANSFER_NON_POSITIVE_QTY::must transfer positive quantity");
   check(quantity.symbol == st.supply.symbol, "ERR::TRANSFER_SYMBOL_MISMATCH::symbol precision mismatch");
   check(memo.size() <= 256, "ERR::TRANSFER_MEMO_TOO_LONG::memo has more than 256 bytes");

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



