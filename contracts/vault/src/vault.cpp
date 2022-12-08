#include <vault.hpp>

#include <cmath>

using std::make_tuple;

void vault::updatestatus(uint8_t transfer_status, uint8_t deposit_status, uint8_t withdraw_status) {
    require_auth(ADMIN_ACCOUNT);
    _config.transfer_status = transfer_status;
    _config.deposit_status  = deposit_status;
    _config.withdraw_status = withdraw_status;
    _configs.set(_config, _self);
}

void vault::createcoll(const name &contract, const symbol &sym, const name &income_account,
                       name fees_account, asset min_quantity, uint16_t income_ratio,
                       uint16_t release_fees, uint16_t refund_ratio) {
    require_auth(ADMIN_ACCOUNT);
    auto collateral = get_collateral(contract, sym, false);
    check(collateral.id == 0, "collateral has inited");

    check(sym == get_supply(contract, sym.code()).symbol, "symbol error");
    check(sym == min_quantity.symbol, "min_quantity symbol error");
    check(income_ratio <= 10000, "income_ratio need less than 10000");
    check(release_fees <= 10000, "income_ratio need less than 10000");
    check(refund_ratio <= 10000, "income_ratio need less than 10000");
    check(is_account(income_account), " income_account does not exist");
    check(is_account(fees_account), " fees_account does not exist");

    symbol issue_symbol
        = symbol(symbol_code(string("S") + sym.code().to_string()), sym.precision());

    collaterals collateraltbl(_self, _self.value);
    uint64_t    new_id = collateraltbl.available_primary_key();
    if (new_id == 0) {
        new_id = 1;
    }
    auto itr = collateraltbl.emplace(_self, [&](auto &a) {
        a.id               = new_id;
        a.deposit_contract = contract;
        a.deposit_symbol   = sym;
        a.issue_symbol     = issue_symbol;
        a.income_account   = income_account;
        a.fees_account     = fees_account;
        a.income_ratio     = income_ratio;
        a.min_quantity     = min_quantity;
        a.release_fees     = release_fees;
        a.refund_ratio     = refund_ratio;
        a.last_income      = asset(0, sym);
        a.total_income     = asset(0, sym);
    });

    // Create SEOS tokens with a total circulation of 1 billion, with the same bit precision
    uint32_t prec_num = pow(10, sym.precision());
    auto data = std::make_tuple(_self, asset(1000000000ULL * prec_num, issue_symbol));
    action(permission_level { _self, "active"_n }, STOKRN_ACCOUNT, "create"_n, data)
        .send();

    auto logdata = std::make_tuple(new_id, contract, sym, issue_symbol,
                                   income_account, fees_account, min_quantity,
                                   income_ratio, release_fees, refund_ratio);
    action(permission_level { _self, "active"_n }, _self, "colupadtelog"_n, logdata)
        .send();
}

void vault::updatecoll(uint64_t collateral_id, const name &income_account,
                       name fees_account, asset min_quantity, uint16_t income_ratio,
                       uint16_t release_fees, uint16_t refund_ratio) {
    require_auth(ADMIN_ACCOUNT);

    check(income_ratio <= 10000, "income_ratio need less than 10000");
    check(release_fees <= 10000, "income_ratio need less than 10000");
    check(refund_ratio <= 10000, "income_ratio need less than 10000");

    collaterals collateraltbl(_self, _self.value);
    auto        itr
        = collateraltbl.require_find(collateral_id, "collateral not found");
    check(itr->deposit_symbol == min_quantity.symbol,
          "min_quantity symbol error");

    collateraltbl.modify(itr, same_payer, [&](auto &a) {
        a.income_account = income_account;
        a.fees_account   = fees_account;
        a.income_ratio   = income_ratio;
        a.min_quantity   = min_quantity;
        a.release_fees   = release_fees;
        a.refund_ratio   = refund_ratio;
    });

    auto logdata = std::make_tuple(collateral_id, itr->deposit_contract,
                                   itr->deposit_symbol, itr->issue_symbol,
                                   income_account, fees_account, min_quantity,
                                   income_ratio, release_fees, refund_ratio);
    action(permission_level { _self, "active"_n }, _self, "colupadtelog"_n, logdata)
        .send();
}

void vault::proxyto(name proxy) {
    require_auth(ADMIN_ACCOUNT);
    // Vote for another agent
    action(permission_level { _self, "active"_n }, EOSIO_ACCOUNT,
           name("voteproducer"), make_tuple(_self, proxy, std::vector<name> {}))
        .send();
}

void vault::buyallrex() {
    if (!has_auth(_self)) {
        require_auth(ADMIN_ACCOUNT);
    }
    buyrex(asset(0, EOS_SYMBOL));
}

void vault::buyrex(asset quantity) {
    if (!has_auth(_self)) {
        require_auth(ADMIN_ACCOUNT);
    }

    // Do not operate until the balance is more than 1 eos
    auto eos_balance = get_balance(EOS_TOKEN_ACCOUNT, _self, EOS_SYMBOL);
    if (quantity.amount == 0) {
        quantity = eos_balance;
    }
    check(eos_balance >= quantity, "eos banlance insufficient to buy rex");
    if (eos_balance.amount < 10000) {
        return;
    }

    // The whole network rex mortgage rate exceeds 85, no longer buy, and to sell all directly
    rex_pool_table rexpool_table(EOSIO_ACCOUNT, EOSIO_ACCOUNT.value);
    auto           itr = rexpool_table.begin();
    auto           pct = itr->total_lent * 100 / itr->total_lendable;
    if (pct >= 85) {
        withdraw_sellrex(_self, asset(0, EOS_SYMBOL), asset(0, EOS_SYMBOL), "sell all REX");
        return;
    }

    deposit_buyrex(quantity);
}

void vault::sellallrex() {
    require_auth(ADMIN_ACCOUNT);

    withdraw_sellrex(_self, asset(0, EOS_SYMBOL), asset(0, EOS_SYMBOL), "sell all REX");
}

void vault::sellrex(asset quantity) {
    require_auth(ADMIN_ACCOUNT);

    auto total_eos = get_rex_eos();

    // check(false, string("rex eos:") + rex_eos.to_string() + string(",total eos:") + _stat.total.to_string() + string(",extra:") + extra.to_string());
    // Sell the excess
    check(quantity <= total_eos, string("not enough rex to sell"));

    withdraw_sellrex(_self, quantity, asset(0, EOS_SYMBOL), "REX reward");
}

void vault::sellnext(name owner, asset quantity, string memo) {
    require_auth(_self);

    action(permission_level { _self, "active"_n }, _self, name("sellnext2"),
           make_tuple(owner, quantity, memo))
        .send();
}

void vault::sellnext2(name owner, asset quantity, string memo) {
    require_auth(_self);

    if (owner != _self) {
        auto balance = get_balance(EOS_TOKEN_ACCOUNT, _self, EOS_SYMBOL);
        if (quantity > balance) {
            if ((quantity - balance).amount < 10) {
                // The error of 0.001 was the acceptable range
                quantity = balance;
            } else {
                print_f("%, % \n", quantity, balance);
                check(false, "error! no enough eos to transfer.");
            }
        }
        // print_f("%, % \n", quantity, balance);
        // check(false, "");
        auto data = std::make_tuple(_self, owner, quantity, memo);
        action(permission_level { _self, "active"_n }, EOS_TOKEN_ACCOUNT, "transfer"_n, data)
            .send();

        // check_for_released(owner);
    }
}

void vault::income() {
    uint64_t unix_ts     = current_time_point().sec_since_epoch();
    uint64_t ten_minutes = minutes(10).to_seconds();
    uint64_t this_time   = unix_ts - (unix_ts % ten_minutes);
    if (_config.last_income_time == this_time) {
        return;
    }
    if (_config.last_income_time > 0) {
        collaterals collateraltbl(_self, _self.value);
        auto        itr = collateraltbl.begin();
        while (itr != collateraltbl.end()) {
            auto period = (this_time - _config.last_income_time) / ten_minutes;
            if (period > 0) {
                uint64_t total_ratio = period * itr->income_ratio;
                if (total_ratio > 10000) {
                    total_ratio = 10000;
                }
                auto quantity = get_balance(itr->deposit_contract,
                                            itr->income_account, itr->deposit_symbol)
                                * total_ratio / 10000L;

                // transfer to self
                if (quantity.amount > 0) {
                    auto data = std::make_tuple(itr->income_account, _self,
                                                quantity, string("award"));
                    action(permission_level { itr->income_account, "active"_n },
                           itr->deposit_contract, "transfer"_n, data)
                        .send();
                }
                // save
                collateraltbl.modify(itr, same_payer, [&](auto &c) {
                    c.last_income = quantity;
                    if (c.total_income.symbol.code() == symbol_code("")) {
                        c.total_income.symbol = itr->deposit_symbol;
                    }
                    c.total_income += quantity;
                });
            }
            itr++;
        }
    }
    _config.last_income_time = this_time;
    _configs.set(_config, _self);
}

void vault::release(name owner) {
    check(_config.withdraw_status == 1, "withdraw has been suspended");
    check_for_released(owner);
}

// logs
void vault::colupadtelog(uint64_t collateral_id, const name &deposit_contract,
                         const symbol &deposit_symbol, const symbol &issue_symbol,
                         const name &income_account, name fees_account,
                         asset min_quantity, uint16_t income_ratio,
                         uint16_t release_fees, uint16_t refund_ratio) {
    require_auth(_self);
}

void vault::depositlog(uint64_t collateral_id, const name &owner,
                       const asset &quantity, uint64_t rate, block_timestamp time) {
    require_auth(_self);
}

void vault::releaselog(uint64_t log_id, uint64_t collateral_id, const name &owner,
                       const asset &quantity, uint64_t rate, block_timestamp time) {
    require_auth(_self);
}

void vault::withdrawlog(uint64_t log_id, uint64_t collateral_id, const name &owner,
                        const asset &withdraw_quantity, const asset &withdraw_award_fees,
                        const asset &withdraw_sys_fees, const asset &refund_award_quantity,
                        const asset &refund_sys_quantity, block_timestamp time) {
    require_auth(_self);
}

// deposit
void vault::on_tokens_transfer(name from, name to, asset quantity, string memo) {
    if (from == _self || to != _self || from == ADMIN_ACCOUNT
        || from == EOSIO_ACCOUNT || from == EOS_REX_ACCOUNT) {
        return;
    }
    auto code = get_first_receiver();
    if (code == STOKRN_ACCOUNT) {
        auto collateral = get_collateral_by_issue_symbol(quantity.symbol);
        do_withdraw(collateral, from, quantity);
    } else {
        auto collateral = get_collateral(code, quantity.symbol);
        do_deposit(collateral, from, quantity);
    }
}

void vault::transfer_token_to(name contract, name to, asset quantity, string memo) {
    // transfer_tokens_to(v[tsf_data] tsfs)
    if (contract == EOS_TOKEN_ACCOUNT && quantity.symbol == EOS_SYMBOL) {
        static auto balance = get_balance(EOS_TOKEN_ACCOUNT, _self, EOS_SYMBOL);
        if (quantity > balance) {
            // If the collateral retrieved is less than the available balance, the corresponding unlockable REX number is retrieved
            // Get the eos difference
            auto diff_eos = quantity - balance;
            print_f("transfer to % quantity %, balance % sellrex: % (%)\n", to,
                    quantity, balance, diff_eos);
            balance.amount = 0;
            diff_eos.amount += 1;   // Prevention of errors
            // sell it and you get the EOS
            withdraw_sellrex(to, diff_eos, quantity, memo);
            return;
        }
        balance -= quantity;
        print_f("transfer quantity: %, balance %\n", quantity, balance);
    }
    // print_f("transfer quantity: %\n", quantity);
    auto data = std::make_tuple(_self, to, quantity, memo);
    action(permission_level { _self, "active"_n }, contract, "transfer"_n, data).send();
}

void vault::do_deposit(s_collateral collateral, const name &owner, const asset &quantity) {
    if (owner == collateral.income_account) {
        return;
    }
    check(_config.deposit_status == 1, "deposit has been suspended");

    check(quantity >= collateral.min_quantity, "deposit too small");

    auto is_eos = collateral.deposit_contract == EOS_TOKEN_ACCOUNT
                  && collateral.deposit_symbol == EOS_SYMBOL;
    uint64_t rate = is_eos ? get_eos_rate(quantity.amount * -1)
                           : get_rate(collateral.deposit_contract, quantity * -1);
    print_f("rate: %, ", rate);

    uint64_t issue_amount = uint128_t(quantity.amount) * RATE_BASE / rate;
    print_f("issue: % ", asset(issue_amount, collateral.issue_symbol));
    // check(false, collateral.deposit_contract.to_string());

    auto issue_quantity = asset(issue_amount, collateral.issue_symbol);
    auto data = std::make_tuple(owner, issue_quantity, string("deposit"));
    action(permission_level { _self, "active"_n }, STOKRN_ACCOUNT, "issue"_n, data)
        .send();

    // deposit
    auto data2 = std::make_tuple(collateral.id, owner, quantity, (uint64_t)rate,
                                 current_block_time());
    action(permission_level { _self, "active"_n }, _self, "depositlog"_n, data2).send();

    // buy rex
    if (is_eos) {
        action(permission_level { _self, "active"_n }, _self, "buyallrex"_n, std::make_tuple())
            .send();
    }
}

// withdraw
void vault::do_withdraw(s_collateral collateral, const name &owner, const asset &quantity) {
    check(_config.withdraw_status == 1, "withdraw has been suspended");

    auto is_eos = collateral.deposit_contract == EOS_TOKEN_ACCOUNT
                  && collateral.deposit_symbol == EOS_SYMBOL;
    uint64_t rate = is_eos ? get_eos_rate(0)
                           : get_rate(collateral.deposit_contract,
                                      asset(0, collateral.deposit_symbol));
    print_f("rate: %, ", rate);

    auto etime = current_time_point() + days(5);   // minutes(5);

    releases releasetbl(_self, owner.value);
    uint64_t release_id = get_log_id();
    releasetbl.emplace(_self, [&](auto &s) {
        s.id       = release_id;
        s.quantity = quantity;
        s.rate     = rate;
        s.time     = block_timestamp(etime);
    });

    auto data = std::make_tuple(release_id, collateral.id, owner, quantity,
                                rate, block_timestamp(etime));
    action(permission_level { _self, "active"_n }, _self, "releaselog"_n, data).send();
}

void vault::deposit_buyrex(asset quantity) {
    check(quantity.symbol == EOS_SYMBOL, "invalid symbol");
    check(quantity.amount > 0, "invalid amount");

    rex_pool_table rexpool_table(EOSIO_ACCOUNT, EOSIO_ACCOUNT.value);
    auto           rex_itr = rexpool_table.begin();

    const int64_t S0         = rex_itr->total_lendable.amount;
    const int64_t R0         = rex_itr->total_rex.amount;
    const int64_t rex_amount = (uint128_t(quantity.amount) * R0) / S0;
    auto          rex_value  = asset(rex_amount, REX_SYMBOL);

    // check(false, string("eos:") + quantity.to_string() + string(", rex:") + rex_value.to_string());

    // Reserve top-up
    action(permission_level { _self, "active"_n }, EOSIO_ACCOUNT,
           name("deposit"), make_tuple(_self, quantity))
        .send();

    // Use reserves to buy rex
    action(permission_level { _self, "active"_n }, EOSIO_ACCOUNT,
           name("buyrex"), make_tuple(_self, quantity))
        .send();
}

void vault::withdraw_sellrex(name user, asset sell_quantity, asset tsf_quantity, string memo) {
    check(sell_quantity.symbol == EOS_SYMBOL, "invalid symbol");
    check(sell_quantity.amount >= 0, "invalid amount");

    rex_pool_table rexpool_table(EOSIO_ACCOUNT, EOSIO_ACCOUNT.value);
    auto           rex_itr   = rexpool_table.begin();
    auto           rex_value = asset(0, REX_SYMBOL);
    auto rate = (double)(rex_itr->total_rex.amount) / rex_itr->total_lendable.amount;
    const int64_t     S0 = rex_itr->total_lendable.amount;
    const int64_t     R0 = rex_itr->total_rex.amount;
    rex_balance_table rexbal_table(EOSIO_ACCOUNT, EOSIO_ACCOUNT.value);
    auto              rexbal_it = rexbal_table.find(_self.value);

    // It is necessary to calculate the available REX that has expired
    static uint64_t matured_rex
        = rexbal_it == rexbal_table.end() ? 0 : calculate_matured_rex(rexbal_it);
    print_f("matured_rex %\n", matured_rex);

    if (sell_quantity.amount == 0) {
        rex_value.amount = matured_rex;
        matured_rex      = 0;
    } else {
        int64_t rex_amount = (uint128_t(sell_quantity.amount) * R0) / S0;
        if (rex_amount > matured_rex) {
            rex_amount  = matured_rex;
            matured_rex = 0;
        } else {
            matured_rex -= rex_amount;
        }
        rex_value.amount = rex_amount;
        // check(false, string("rex_value:") + rex_value.to_string()+ ",rate:" + to_string(rate));
    }
    sell_quantity.amount = (uint128_t(rex_value.amount) * S0) / R0;
    // if (user != name("tester1")) {
    // check(false, string("sell rex:") + rex_value.to_string() +  string(", matured rex:") + asset(matured_rex, REX_SYMBOL).to_string() +  string(", sell eos:") + sell_quantity.to_string());
    // }
    if (rex_value.amount <= 0) {
        return;
    }

    // Use reserves to buy rex
    action(permission_level { _self, "active"_n }, EOSIO_ACCOUNT,
           name("sellrex"), make_tuple(_self, rex_value))
        .send();

    // The reserve is withdrawn
    action(permission_level { _self, "active"_n }, EOSIO_ACCOUNT,
           name("withdraw"), make_tuple(_self, sell_quantity))
        .send();

    // To withdraw cash to the user, the next action needs to be called, otherwise the eos will not arrive
    action(permission_level { _self, "active"_n }, _self, name("sellnext"),
           make_tuple(user, tsf_quantity, memo))
        .send();
}

void vault::check_for_released(const name &owner) {
    // todo: refund here, when release, here save release_rate
    releases releasetbl(_self, owner.value);
    auto     itr      = releasetbl.begin();
    auto     now_time = current_time_point();
    if (itr != releasetbl.end()) {
        if (itr->time.to_time_point() > now_time) {
            return;
        }
        auto collateral = get_collateral_by_issue_symbol(itr->quantity.symbol);

        auto is_eos = collateral.deposit_contract == EOS_TOKEN_ACCOUNT
                      && collateral.deposit_symbol == EOS_SYMBOL;

        uint128_t rate0 = itr->rate;
        uint128_t rate1 = is_eos ? get_eos_rate(0)
                                 : get_rate(collateral.deposit_contract,
                                            asset(0, collateral.deposit_symbol));
        // print_f("rate0: % , rate1: % \n", rate0, rate1);
        if (rate1 < rate0) {
            rate1 = rate0;
        }

        int64_t refund0_amount = uint128_t(itr->quantity.amount) * rate0 / RATE_BASE;
        int64_t refund1_amount = uint128_t(itr->quantity.amount) * rate1 / RATE_BASE;
        print_f("refund0_amount: % , quantity: % rate0: %, rate1: %, RATE_BASE "
                "% \n",
                refund0_amount, itr->quantity.amount, rate0, rate1, RATE_BASE);

        auto data1 = std::make_tuple(itr->quantity, string("withdraw retire"));
        action(permission_level { _self, "active"_n }, STOKRN_ACCOUNT, "retire"_n, data1)
            .send();

        auto withdraw_quantity = asset(refund0_amount, collateral.deposit_symbol);
        // print_f("withdraw_quantity: %\n", withdraw_quantity);
        auto withdraw_fees = asset(refund0_amount * collateral.release_fees / 10000,
                                   collateral.deposit_symbol);

        withdraw_quantity -= withdraw_fees;
        if (withdraw_quantity.amount > 0) {
            transfer_token_to(collateral.deposit_contract, owner,
                              withdraw_quantity, string("withdraw"));
        }

        auto withdraw_to_award_fees = withdraw_fees * collateral.refund_ratio / 10000;
        auto withdraw_to_sys_fees = withdraw_fees - withdraw_to_award_fees;
        if (withdraw_to_award_fees.amount > 0) {
            transfer_token_to(collateral.deposit_contract, collateral.income_account,
                              withdraw_to_award_fees, string("withdraw fees"));
        }
        if (withdraw_to_sys_fees.amount > 0) {
            transfer_token_to(collateral.deposit_contract, collateral.fees_account,
                              withdraw_to_sys_fees, string("withdraw fees"));
        }

        auto refund_quantity
            = asset(refund1_amount - refund0_amount, collateral.deposit_symbol);
        auto refund_to_award_quantity = refund_quantity * collateral.refund_ratio / 10000;
        auto refund_to_sys_quantity = refund_quantity - refund_to_award_quantity;
        if (refund_to_award_quantity.amount > 0) {
            transfer_token_to(collateral.deposit_contract, collateral.income_account,
                              refund_to_award_quantity, string("refund"));
        }
        if (refund_to_sys_quantity.amount > 0) {
            transfer_token_to(collateral.deposit_contract, collateral.fees_account,
                              refund_to_sys_quantity, string("refund"));
        }
        print_f("refund_quantity: %\n", refund_quantity);
        auto log_id = itr->id;
        itr         = releasetbl.erase(itr);

        // check(false, "~~");

        auto data = std::make_tuple(log_id, collateral.id, owner,
                                    withdraw_quantity, withdraw_to_award_fees,
                                    withdraw_to_sys_fees, refund_to_award_quantity,
                                    refund_to_sys_quantity, current_block_time());
        action(permission_level { _self, "active"_n }, _self, "withdrawlog"_n, data)
            .send();
    }
}