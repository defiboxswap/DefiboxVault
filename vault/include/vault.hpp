#pragma once
#include <eosio/eosio.hpp>
#include <eosio/asset.hpp>
#include <eosio/system.hpp>
#include <eosio/singleton.hpp>

#include <defines.hpp>
#include <tables.hpp>

using namespace eosio;
using std::string;

class [[eosio::contract("vault")]] vault : public contract {
public:
      vault(name receiver, name code, datastream<const char *> ds): contract(receiver, code, ds),
         _configs(_self, _self.value) {
            if (_configs.exists()) {
               _config = _configs.get();
            } else {
               _config.last_income_time = 0;
               _config.transfer_status = 1;
               _config.deposit_status = 1;
               _config.withdraw_status = 1;
               _config.log_id = 0;
               _configs.set(_config, _self);
            }
         }

      [[eosio::action]] void updatestatus(uint8_t transfer_status, uint8_t deposit_status, uint8_t withdraw_status);
      [[eosio::action]] void createcoll(const name &contract, const symbol &sym, const name &income_account, name fees_account, asset min_quantity, uint16_t income_ratio, uint16_t release_fees, uint16_t refund_ratio);
      [[eosio::action]] void updatecoll(uint64_t collateral_id, const name &income_account, name fees_account, asset min_quantity, uint16_t income_ratio, uint16_t release_fees, uint16_t refund_ratio);
      [[eosio::action]] void proxyto(name proxy);
      [[eosio::action]] void buyallrex();
      [[eosio::action]] void buyrex(asset quantity);
      [[eosio::action]] void sellallrex();
      [[eosio::action]] void sellrex(asset quantity);
      [[eosio::action]] void sellnext(name owner, asset quantity, string memo);
      [[eosio::action]] void sellnext2(name owner, asset quantity, string memo);
      
      [[eosio::action]] void income();
      [[eosio::action]] void release(name owner);

      // [[eosio::action]] void printrate() {
      //    auto rate = get_eos_rate(0);
      //    print_f("rate: %\n", rate);
      //    check(false, "printer");
      // }

      // logs
      [[eosio::action]] void colupadtelog(uint64_t collateral_id, const name &deposit_contract, const symbol &deposit_symbol, const symbol &issue_symbol, const name &income_account, name fees_account, asset min_quantity, uint16_t income_ratio, uint16_t release_fees, uint16_t refund_ratio);
      [[eosio::action]] void depositlog(uint64_t collateral_id, const name &owner, const asset &quantity, uint64_t rate, block_timestamp time);
      [[eosio::action]] void releaselog(uint64_t log_id, uint64_t collateral_id, const name &owner, const asset &quantity, uint64_t rate, block_timestamp time);
      [[eosio::action]] void withdrawlog(uint64_t log_id, uint64_t collateral_id, const name &owner, const asset &withdraw_quantity, const asset &withdraw_award_fees, const asset &withdraw_sys_fees, const asset &refund_award_quantity, const asset &refund_sys_quantity, block_timestamp time);

      // notify
      [[eosio::on_notify("*::transfer")]]
      void on_tokens_transfer(name from, name to, asset quantity, string memo);

      // utils
      static asset get_supply(const name &token_contract_account, const symbol_code &sym_code) {
         stats statstable(token_contract_account, sym_code.raw());
         const auto &st = statstable.get(sym_code.raw());
         return st.supply;
      }

      static asset get_balance(const name &token_contract_account, const name &owner, const symbol &sym) {
         accounts accountstable(token_contract_account, owner.value);
         auto ac = accountstable.find(sym.code().raw());
         if (ac != accountstable.end()) {
            return ac->balance;
         } else {
            return asset(0, sym);
         }
      }

   private:
      struct [[eosio::table]] s_release {
         uint64_t id;
         asset quantity;
         uint64_t rate;
         block_timestamp time;
         uint64_t primary_key() const { return id; }
      };

      struct [[eosio::table]] s_collateral {
         uint64_t id;
         name deposit_contract;
         symbol deposit_symbol;
         symbol issue_symbol;
         asset last_income;
         asset total_income;
         uint16_t income_ratio = 10;  // 转账万分比，默认10
         name income_account;
         asset min_quantity;
         name fees_account;
         uint16_t release_fees = 30;
         uint16_t refund_ratio = 5000;
         uint64_t primary_key() const { return id; }
      };

      struct [[eosio::table]] config {
         uint64_t last_income_time;
         uint8_t transfer_status;
         uint8_t deposit_status;
         uint8_t withdraw_status;
         uint64_t log_id;
      };

  
      typedef eosio::multi_index<"releases"_n, s_release> releases;
      typedef eosio::multi_index<"collaterals"_n, s_collateral> collaterals;
      typedef eosio::singleton<"config"_n, config> configs;

      configs _configs;
      config _config;

      void transfer_token_to(name contract, name to, asset quantity, string memo);

      void do_deposit(s_collateral collateral, const name &owner, const asset &quantity);
      void do_withdraw(s_collateral collateral, const name &owner, const asset &quantity);

      void deposit_buyrex(asset quantity);
      void withdraw_sellrex(name user, asset sell_quantity, asset tsf_quantity, string memo);

      // if there are some tokens to release, release it
      void check_for_released(const name &owner);

       asset get_rex_eos() {
         rex_pool_table rexpool_table(EOSIO_ACCOUNT, EOSIO_ACCOUNT.value);
         auto rex_itr = rexpool_table.begin();
         
         rex_balance_table rexbal_table(EOSIO_ACCOUNT, EOSIO_ACCOUNT.value);
         auto rexbal_it = rexbal_table.find(_self.value);

         auto rex_eos = asset(0, EOS_SYMBOL);
         if (rexbal_it == rexbal_table.end()) {
            return rex_eos;
         }

         const int64_t S0 = rex_itr->total_lendable.amount;
         const int64_t R0 = rex_itr->total_rex.amount;
         rex_eos.amount   = ( uint128_t(rexbal_it->rex_balance.amount) * S0 ) / R0;

         return rex_eos;
      }

      uint64_t get_eos_rate(int64_t amount) {
         uint64_t rex_eos_amount = get_rex_eos().amount;
         
         uint128_t eos_amount = get_balance(EOS_TOKEN_ACCOUNT, _self, EOS_SYMBOL).amount;
         eos_amount += rex_eos_amount;
         eos_amount += amount;

         auto vault_supply = get_supply(STOKRN_ACCOUNT, symbol_code("SEOS"));

         print_f("rex eos: %, total eos: % seos supply: % \n", asset(rex_eos_amount, EOS_SYMBOL), asset(eos_amount, EOS_SYMBOL), vault_supply);

         if (vault_supply.amount == 0) {
            return RATE_BASE;
         }

         return eos_amount * RATE_BASE / vault_supply.amount;
      }

      uint64_t get_rate(name contract, asset quantity) {
         
         auto balance = get_balance(contract, _self, quantity.symbol);
         balance += quantity;

         symbol_code issue_sc = symbol_code(string("S") + quantity.symbol.code().to_string());
         auto vault_supply = get_supply(STOKRN_ACCOUNT, issue_sc);

         // check(false, "balance " + balance.to_string() + " supply " + vault_supply.to_string());

         if (vault_supply.amount == 0) {
            return RATE_BASE;
         }

         return uint128_t(balance.amount) * RATE_BASE / vault_supply.amount;
      }
      
      s_collateral get_collateral(name contract, symbol sym, bool error_when_empty = true) {
         collaterals collateraltbl(_self, _self.value);
         auto itr = collateraltbl.begin();
         while (itr != collateraltbl.end()) {
            if (itr->deposit_contract == contract && itr->deposit_symbol == sym) {
               break;
            }
            itr++;
         }
         
         check(!error_when_empty || itr != collateraltbl.end(), "deposit token not found");
         if (!error_when_empty && itr == collateraltbl.end()) {
            s_collateral s;
            s.id = 0;
            return s;
         }

         return *itr;
      }

      s_collateral get_collateral_by_id(uint64_t id) {
         collaterals collateraltbl(_self, _self.value);
         auto itr = collateraltbl.require_find(id, "collateral not found");
         return *itr;
      }

      s_collateral get_collateral_by_issue_symbol(symbol sym) {
         collaterals collateraltbl(_self, _self.value);
         auto itr = collateraltbl.begin();
         while (itr != collateraltbl.end()) {
            if (itr->issue_symbol == sym) {
               break;
            }
            itr++;
         }
         check(itr != collateraltbl.end(), "collateral not found");

         return *itr;
      }

      uint64_t calculate_matured_rex(rex_balance_table::const_iterator rexbal_it) {
        uint64_t matured_rex = rexbal_it->matured_rex;
         auto now = time_point_sec(current_time_point());
         auto mr_itr = rexbal_it->rex_maturities.begin();
         while (mr_itr != rexbal_it->rex_maturities.end()) {
            if (mr_itr->first <= now) {
                  matured_rex += mr_itr->second;
            }
            mr_itr++;
         }
         return matured_rex;
      }

      uint64_t get_log_id() {
         _config.log_id++;
         _configs.set(_config, _self);
         return _config.log_id;
      }

};