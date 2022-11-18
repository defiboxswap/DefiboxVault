#pragma once
#include <eosio/eosio.hpp>

using namespace eosio;

static constexpr name EOSIO_ACCOUNT{"eosio"_n};
static constexpr name EOS_TOKEN_ACCOUNT{"eosio.token"_n};
static constexpr name EOS_REX_ACCOUNT{"eosio.rex"_n};
static constexpr name ADMIN_ACCOUNT{"admin.defi"_n};
static constexpr name STOKRN_ACCOUNT{"stoken.defi"_n};

static constexpr symbol EOS_SYMBOL = symbol("EOS", 4);
static constexpr symbol REX_SYMBOL = symbol("REX", 4);

static const uint64_t RATE_BASE = 100000000LL;