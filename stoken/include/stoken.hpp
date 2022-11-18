#pragma once
#include <eosio/eosio.hpp>
#include <eosio/asset.hpp>
#include <eosio/system.hpp>
#include <eosio/singleton.hpp>

#include <utils.hpp>

using namespace eosio;
using std::string;

class [[eosio::contract("stoken")]] stoken : public contract {
public:
      using contract::contract;

      // actions for token
      [[eosio::action]] void create(const name &issuer, const asset &maximum_supply);
      [[eosio::action]] void issue(const name &to, const asset &quantity, const string &memo);
      [[eosio::action]] void retire(const asset &quantity, const string &memo);
      [[eosio::action]] void transfer(const name &from, const name &to, const asset &quantity, const string &memo);
      [[eosio::action]] void open(const name &owner, const symbol &symbol, const name &ram_payer);
      [[eosio::action]] void close(const name &owner, const symbol &symbol);

      [[eosio::action]] void transferlog(const name &from, const name &to, const asset &quantity, const asset &from_balance, const asset &to_balance);

      // utils
      static asset get_supply(const name &token_contract_account, const symbol_code &sym_code) {
         stats statstable(token_contract_account, sym_code.raw());
         const auto &st = statstable.get(sym_code.raw());
         return st.supply;
      }

      static asset get_balance(const name &token_contract_account, const name &owner, const symbol_code &sym_code) {
         accounts accountstable(token_contract_account, owner.value);
         const auto &ac = accountstable.get(sym_code.raw());
         return ac.balance;
      }

   private:
      struct [[eosio::table]] s_account {
         asset balance;
         uint64_t primary_key() const { return balance.symbol.code().raw(); }
      };

      struct [[eosio::table]] s_stat {
         asset supply;
         asset max_supply;
         name issuer;

         uint64_t primary_key() const { return supply.symbol.code().raw(); }
      };

     
      typedef eosio::multi_index<"accounts"_n, s_account> accounts;
      typedef eosio::multi_index<"stat"_n, s_stat> stats;

      asset sub_balance(const name &owner, const asset &value);
      asset add_balance(const name &owner, const asset &value, const name &ram_payer);

};