#pragma once
#include <eosio/eosio.hpp>

using namespace eosio;

static constexpr name VAULT_ACCOUNT{"vault.defi"_n};

struct config {
    uint64_t last_income_time;
    uint8_t transfer_status;
    uint8_t deposit_status;
    uint8_t withdraw_status;
};
typedef eosio::singleton<"config"_n, config> configs;