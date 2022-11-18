#pragma once
#include <eosio/eosio.hpp>
#include <eosio/asset.hpp>

using namespace eosio;

struct rex_pool {
      uint8_t    version = 0;
      asset      total_lent;
      asset      total_unlent;
      asset      total_rent;
      asset      total_lendable;
      asset      total_rex;
      asset      namebid_proceeds;
      uint64_t   loan_num = 0;

      uint64_t primary_key()const { return 0; }
};

typedef eosio::multi_index< "rexpool"_n, rex_pool > rex_pool_table;

struct rex_balance {
    uint8_t version = 0;
    name    owner;
    asset   vote_stake;
    asset   rex_balance;
    int64_t matured_rex = 0;
    std::deque<std::pair<time_point_sec, int64_t>> rex_maturities;

    uint64_t primary_key()const { return owner.value; }
};

typedef eosio::multi_index< "rexbal"_n, rex_balance > rex_balance_table;


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