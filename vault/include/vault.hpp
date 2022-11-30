#pragma once
#include <eosio/asset.hpp>
#include <eosio/eosio.hpp>
#include <eosio/singleton.hpp>
#include <eosio/system.hpp>

#include <defines.hpp>
#include <tables.hpp>

using namespace eosio;
using std::string;

class [[eosio::contract("vault")]] vault : public contract {
  public:
    vault(name receiver, name code, datastream<const char *> ds)
        : contract(receiver, code, ds), _configs(_self, _self.value) {
        if (_configs.exists()) {
            _config = _configs.get();
        } else {
            _config.last_income_time = 0;
            _config.transfer_status  = 1;
            _config.deposit_status   = 1;
            _config.withdraw_status  = 1;
            _config.log_id           = 0;
            _configs.set(_config, _self);
        }
    }
    /**
     * ## ACTION `updatestatus`
     *
     * > Modifying Global Status.
     *
     * - **authority**: `admin.defi`
     *
     * ### params
     *
     * - `{uint8_t} transfer_status` - transfer status (`0: suspended 1: open`)
     * - `{uint8_t} deposit_status` - deposit status (`0: suspended 1: open`)
     * - `{uint8_t} withdraw_status` - withdraw status (`0: suspended 1: open`)
     *
     * ### example
     *
     *
     * ```bash
     * $ cleos push action vault.defi updatestatus '[1, 1, 1]' -p admin.defi
     * ```
     */
    [[eosio::action]] void updatestatus(uint8_t transfer_status, uint8_t deposit_status,
                                        uint8_t withdraw_status);
    /**
     * ## ACTION `createcoll`
     *
     * > Create the collateral.
     *
     * - **authority**: `admin.defi`
     *
     * ### params
     *
     * - `{name} contract` - collateral token contract
     * - `{name} sym` - collateral token symbol
     * - `{name} income_account` - used to store reward accounts
     * - `{name} fees_account` - receiving service charge Account
     * - `{asset} min_quantity` - minimum deposit quantity
     * - `{uint16_t} income_ratio` - percentage of collateral->income_account transferred(pips 100/10000 of 1%)
     * - `{uint16_t} release_fees` - percentage of withdraw service fee(pips 100/10000 of 1%)
     * - `{uint16_t} refund_ratio` - the proportion of the withdrawal fee returned to the collateral->income_account(pips 100/10000 of 1%)
     *
     * ### example
     *
     * ```bash
     * $ cleos push action vault.defi createcoll '["eosio.token", "4,EOS", "award.defi", "fees.defi", "0.1000 EOS", "10", "30", "5000"]' -p admin.defi
     * ```
     */
    [[eosio::action]] void createcoll(const name &contract, const symbol &sym,
                                      const name &income_account, name fees_account,
                                      asset min_quantity, uint16_t income_ratio,
                                      uint16_t release_fees, uint16_t refund_ratio);

    /**
     * ## ACTION `updatecoll`
     *
     * > Modify the configuration of the collateral.
     *
     * - **authority**: `admin.defi`
     *
     * ### params
     *
     * - `{uint64_t} collateral_id` - the collateral id
     * - `{name} income_account` - used to store reward accounts
     * - `{name} fees_account` - receiving service charge Account
     * - `{asset} min_quantity` - Minimum deposit quantity
     * - `{uint16_t} income_ratio` - percentage of collateral->income_account transferred(pips 100/10000 of 1%)
     * - `{uint16_t} release_fees` - percentage of withdraw service fee(pips 100/10000 of 1%)
     * - `{uint16_t} refund_ratio` - the proportion of the withdrawal fee returned to the collateral->income_account(pips 100/10000 of 1%)
     *
     * ### example
     *
     * ```bash
     * $ cleos push action vault.defi updatecoll '[1, "award.defi", "fees.defi", "10", "0.2000 EOS", "10", "30", "5000"]' -p admin.defi
     * ```
     */
    [[eosio::action]] void updatecoll(uint64_t collateral_id,
                                      const name &income_account, name fees_account,
                                      asset min_quantity, uint16_t income_ratio,
                                      uint16_t release_fees, uint16_t refund_ratio);
    /**
     * ## ACTION `proxyto`
     *
     * > Vote for another agent.
     *
     * - **authority**: `admin.defi`
     *
     * ### params
     *
     * - `{name} proxy` - account that accept votes
     *
     * ### example
     *
     * ```bash
     * $ cleos push action vault.defi proxyto '["voteto"]' -p admin.defi
     * ```
     */
    [[eosio::action]] void proxyto(name proxy);

    /**
     * ## ACTION `buyallrex`
     *
     * > Swap all the collateral for rex.
     *
     * - **authority**: `get_self()` or `admin.defi`
     *
     * ```bash
     * $ cleos push action vault.defi buyallrex '[]' -p admin.defi
     * ```
     *
     */
    [[eosio::action]] void buyallrex();
    /**
     * ## ACTION `buyrex`
     *
     * > Swap {{quantity}} the collateral for rex.
     *
     * - **authority**: `get_self()` or `admin.defi`
     *
     * ### params
     *
     * - `{asset} quantity` - oracle account
     *
     * ### example
     *
     * ```bash
     * $ cleos push action vault.defi buyrex '["10.0000 EOS"]' -p admin.defi
     * ```
     */
    [[eosio::action]] void buyrex(asset quantity);

    /**
     * ## ACTION `sellallrex`
     *
     * > Sell all the Rexes.
     *
     * - **authority**: `admin.defi`
     *
     * ```bash
     * $ cleos push action vault.defi sellallrex '[]' -p admin.defi
     * ```
     */
    [[eosio::action]] void sellallrex();

    /**
     * ## ACTION `sellrex`
     *
     * > Sell {{quantity}} of rex.
     *
     * - **authority**: `admin.defi`
     *
     * ### params
     *
     * - `{asset} quantity` - amount of rex to sell
     *
     * ### example
     *
     * ```bash
     * $ cleos push action vault.defi sellrex '["10.0000 EOS"]' -p admin.defi
     * ```
     */
    [[eosio::action]] void sellrex(asset quantity);
    /**
     * ## ACTION `sellnext`
     *
     * > Sell {{quantity}} of rex for {{owner}}.
     *
     * - **authority**: `get_self()`
     *
     * ### params
     *
     * - `{name} owner` - the deposit account
     * - `{asset} quantity` - amount of rex to sell
     * - `{string} memo` - the memo string to accompany the transaction.
     *
     * ### example
     *
     * ```bash
     * $ cleos push action vault.defi sellnext '["depositowner", "10.0000 EOS", ""]' -p vault.defi
     * ```
     */
    [[eosio::action]] void sellnext(name owner, asset quantity, string memo);
    /**
     * ## ACTION `sellnext2`
     *
     * > Sell {{quantity}} of rex for {{owner}}.
     *
     * - **authority**: `get_self()`
     *
     * ### params
     *
     * - `{name} owner` - the deposit account
     * - `{name} quantity` - amount of rex to sell
     * - `{name} memo` - the memo string to accompany the transaction.
     *
     * ### example
     *
     * ```bash
     * $ cleos push action vault.defi sellnext2 '["depositowner", "10.0000 EOS", ""]' -p vault.defi
     * ```
     */
    [[eosio::action]] void sellnext2(name owner, asset quantity, string memo);

    /**
     * ## ACTION `income`
     *
     * > Transfer from collateral->income_account to vault contract (every 10 minutes).
     *
     * - **authority**: `anyone`
     *
     * ```bash
     * $ cleos push action vault.defi income '[]' -p any
     * ```
     */
    [[eosio::action]] void income();

    /**
     * ## ACTION `release`
     *
     * > The mortgaged property to be withdrawn and deposited after maturity.
     *
     * - **authority**: `owner`
     *
     * ### params
     *
     * - `{name} owner` - the deposit account
     *
     * ### example
     *
     * ```bash
     * $ cleos push action vault.defi release '[mydeposit]' -p mydeposit
     * ```
     */
    [[eosio::action]] void release(name owner);

    /**
     * ## ACTION `colupadtelog`
     *
     * > Generates a log when an collateral is created or modified.
     *
     * - **authority**: `get_self()`
     *
     * ### params
     *
     * - `{uint64_t} collateral_id` - the collateral id
     * - `{name} deposit_contract` - collateral token contract
     * - `{symbl} deposit_symbol` - collateral token symbol
     * - `{symbl} issue_symbol` - the token to be issue
     * - `{name} income_account` - used to store reward accounts
     * - `{name} fees_account` - receiving service charge Account
     * - `{asset} min_quantity` - Minimum deposit quantity
     * - `{uint16_t} income_ratio` - Percentage of collateral->income_account transferred
     * - `{uint16_t} release_fees` - withdraw service fee
     * - `{uint16_t} refund_ratio` - The proportion of the withdrawal fee returned to the collateral->income_account
     *
     * ### example
     *
     * ```json
     * {
     *    "collateral_id": "2",
     *    "deposit_contract": "tethertether",
     *    "deposit_symbol": "4,USDT",
     *    "issue_symbol": "4,SUSDT",
     *    "income_account": "award.defi",
     *    "fees_account": "fees.defi",
     *    "min_quantity": "0.1000 USDT",
     *    "income_ratio": 30,
     *    "release_fees": 30,
     *    "refund_ratio": 500
     * }
     * ```
     */
    [[eosio::action]] void colupadtelog(
        uint64_t collateral_id, const name &deposit_contract,
        const symbol &deposit_symbol, const symbol &issue_symbol,
        const name &income_account, name fees_account, asset min_quantity,
        uint16_t income_ratio, uint16_t release_fees, uint16_t refund_ratio);

    /**
     * ## ACTION `depositlog`
     *
     * >  Generates a log each time deposit.
     *
     * - **authority**: `get_self()`
     *
     * ### params
     *
     * - `{uint64_t} collateral_id` - the collateral id
     * - `{name} owner` - the deposit acount
     * - `{asset} quantity` - amount of deposit
     * - `{uint64_t} rate` - ratio of exchange
     * - `{block_timestamp} time` - the time of deposit
     *
     * ### example
     *
     * ```json
     * {
     *    "collateral_id": "2",
     *    "owner": "depositowner",
     *    "quantity": "100.0000 USDT",
     *    "rate": "199690010",
     *    "time": "2022-11-29T06:10:15.500"
     * }
     * ```
     */
    [[eosio::action]] void depositlog(uint64_t collateral_id, const name &owner,
                                      const asset &quantity, uint64_t rate,
                                      block_timestamp time);
    /**
     * ## ACTION `releaselog`
     *
     * > Generates a log when any user calls check_for_released.
     *
     * - **authority**: `get_self()`
     *
     * ### params
     *
     * - `{name} log_id` - table `releases` primary key
     * - `{uint64_t} collateral_id` - the collateral id
     * - `{name} owner` - the deposit acount
     * - `{asset} quantity` - amount of deposit
     * - `{uint64_t} rate` - ratio of exchange
     * - `{block_timestamp} time` - the time of release
     *
     * ### example
     *
     * ```json
     * {
     *     "log_id": "22",
     *     "collateral_id": "2",
     *     "owner": "depositowner",
     *     "quantity": "30.0000 SUSDT",
     *     "rate": "199690012",
     *     "time": "2022-12-04T06:14:52.500"
     * }
     * ```
     */
    [[eosio::action]] void releaselog(uint64_t log_id, uint64_t collateral_id,
                                      const name &owner, const asset &quantity,
                                      uint64_t rate, block_timestamp time);
    /**
     * ## ACTION `withdrawlog`
     *
     * > Generates a log when the user draws the collateral.
     *
     * - **authority**: `get_self()`
     *
     * ### params
     *
     * - `{uint64_t} log_id` - table `releases` primary key
     * - `{uint64_t} collateral_id` - the collateral id
     * - `{name} owner` - the deposit acount
     * - `{asset} withdraw_quantity` - amount of withdraw
     * - `{asset} withdraw_award_fees` - amount of the withdrawal fee returned to the collateral->income_account
     * - `{asset} withdraw_sys_fees` - amount of the withdrawal fee to the collateral->fees_account
     * - `{asset} refund_award_quantity` - oracle account
     * - `{asset} refund_sys_quantity` - oracle account
     * - `{block_timestamp} time` - the block timestamp of the transaction
     *
     * ### example
     *
     * ```json
     * {
     *   "log_id": "13",
     *   "collateral_id": "1",
     *   "owner": "depositowner",
     *   "withdraw_quantity": "4000016.8402 EOS",
     *   "withdraw_award_fees": "0.0000 EOS",
     *   "withdraw_sys_fees": "0.0000 EOS",
     *   "refund_award_quantity": "0.0000 EOS",
     *   "refund_sys_quantity": "2449.4308 EOS",
     *   "time": "2022-11-29T02:35:03.000"
     * }
     * ```
     */
    [[eosio::action]] void withdrawlog(
        uint64_t log_id, uint64_t collateral_id, const name &owner,
        const asset &withdraw_quantity, const asset &withdraw_award_fees,
        const asset &withdraw_sys_fees, const asset &refund_award_quantity,
        const asset &refund_sys_quantity, block_timestamp time);

    // notify
    [[eosio::on_notify("*::transfer")]] void on_tokens_transfer(
        name from, name to, asset quantity, string memo);

    // utils
    static asset get_supply(const name &token_contract_account, const symbol_code &sym_code) {
        stats       statstable(token_contract_account, sym_code.raw());
        const auto &st = statstable.get(sym_code.raw());
        return st.supply;
    }

    static asset get_balance(const name &token_contract_account,
                             const name &owner, const symbol &sym) {
        accounts accountstable(token_contract_account, owner.value);
        auto     ac = accountstable.find(sym.code().raw());
        if (ac != accountstable.end()) {
            return ac->balance;
        } else {
            return asset(0, sym);
        }
    }

  private:
    /**
     * ## TABLE `releases`
     *
     * ### params
     *
     * - `{uint64_t} id` - primary key
     * - `{asset} quantity` - the amount that can be released
     * - `{uint64_t} rate` - ratio of exchange
     * - `{block_timestamp} time` - time when the collateral can be released
     *
     * ### example
     *
     * ```json
     * {
     *     "id": 17,
     *     "quantity": "1000.0000 SUSDT",
     *     "rate": 199578590,
     *     "time": "2022-12-03T10:13:07.000"
     *  }
     * ```
     */
    struct [[eosio::table]] s_release {
        uint64_t        id;
        asset           quantity;
        uint64_t        rate;
        block_timestamp time;
        uint64_t        primary_key() const { return id; }
    };
    /**
     * ## TABLE `collaterals`
     *
     * ### params
     *
     * - `{uint64_t} id` - the collateral id
     * - `{name} deposit_contract` - collateral token contract
     * - `{symbl} deposit_symbol` - collateral token symbol
     * - `{symbl} issue_symbol` - the token to be issue
     * - `{asset} last_income` - last transfer from collateral->income account
     * - `{asset} total_income` - transfer total quantity from collateral->income account
     * - `{uint16_t} income_ratio` - Percentage of collateral->income_account transferred
     * - `{name} income_account` - used to store reward accounts
     * - `{asset} min_quantity` - Minimum deposit quantity
     * - `{name} fees_account` - receiving service charge Account
     * - `{uint16_t} release_fees` - withdraw service fee
     * - `{uint16_t} refund_ratio` - The proportion of the withdrawal fee returned to the collateral->income_account
     *
     * ### example
     *
     * ```json
     * {
     *      "id": 1,
     *      "deposit_contract": "eosio.token",
     *      "deposit_symbol": "4,EOS",
     *      "issue_symbol": "4,SEOS",
     *      "last_income": "1.3344 EOS",
     *      "total_income": "5208.1385 EOS",
     *      "income_ratio": 50,
     *      "income_account": "award.defi",
     *      "min_quantity": "0.1000 EOS",
     *      "fees_account": "vfees.defi",
     *      "release_fees": 0,
     *      "refund_ratio": 0
     * }
     * ```
     */
    struct [[eosio::table]] s_collateral {
        uint64_t id;
        name     deposit_contract;
        symbol   deposit_symbol;
        symbol   issue_symbol;
        asset    last_income;
        asset    total_income;
        uint16_t income_ratio = 10;
        name     income_account;
        asset    min_quantity;
        name     fees_account;
        uint16_t release_fees = 30;
        uint16_t refund_ratio = 5000;
        uint64_t primary_key() const { return id; }
    };
    /**
     * ## TABLE `configs`
     *
     * ### params
     *
     * - `{uint64_t} last_income_time` - (primary key) token symbol
     * - `{uint8_t} transfer_status` - transfer status (`0: suspended 1: open`)
     * - `{uint8_t} deposit_status` - deposit status (`0: suspended 1: open`)
     * - `{uint8_t} withdraw_status` - withdraw status (`0: suspended 1: open`)
     * - `{uint64_t} log_id` - Save the latest id of the `releases` table
     *
     * ### example
     *
     * ```json
     * {
     *    "last_income_time": 1669710600,
     *    "transfer_status": 1,
     *    "deposit_status": 1,
     *    "withdraw_status": 1,
     *    "log_id": 22
     * }
     * ```
     */
    struct [[eosio::table]] config {
        uint64_t last_income_time;
        uint8_t  transfer_status;
        uint8_t  deposit_status;
        uint8_t  withdraw_status;
        uint64_t log_id;
    };

    typedef eosio::multi_index<"releases"_n, s_release>       releases;
    typedef eosio::multi_index<"collaterals"_n, s_collateral> collaterals;
    typedef eosio::singleton<"config"_n, config>              configs;

    configs _configs;
    config  _config;

    void transfer_token_to(name contract, name to, asset quantity, string memo);

    void do_deposit(s_collateral collateral, const name &owner, const asset &quantity);
    void do_withdraw(s_collateral collateral, const name &owner, const asset &quantity);

    void deposit_buyrex(asset quantity);
    void withdraw_sellrex(name user, asset sell_quantity, asset tsf_quantity, string memo);

    // if there are some tokens to release, release it
    void check_for_released(const name &owner);

    asset get_rex_eos() {
        rex_pool_table rexpool_table(EOSIO_ACCOUNT, EOSIO_ACCOUNT.value);
        auto           rex_itr = rexpool_table.begin();

        rex_balance_table rexbal_table(EOSIO_ACCOUNT, EOSIO_ACCOUNT.value);
        auto              rexbal_it = rexbal_table.find(_self.value);

        auto rex_eos = asset(0, EOS_SYMBOL);
        if (rexbal_it == rexbal_table.end()) {
            return rex_eos;
        }

        const int64_t S0 = rex_itr->total_lendable.amount;
        const int64_t R0 = rex_itr->total_rex.amount;
        rex_eos.amount   = (uint128_t(rexbal_it->rex_balance.amount) * S0) / R0;

        return rex_eos;
    }

    uint64_t get_eos_rate(int64_t amount) {
        uint64_t rex_eos_amount = get_rex_eos().amount;

        uint128_t eos_amount = get_balance(EOS_TOKEN_ACCOUNT, _self, EOS_SYMBOL).amount;
        eos_amount += rex_eos_amount;
        eos_amount += amount;

        auto vault_supply = get_supply(STOKRN_ACCOUNT, symbol_code("SEOS"));

        print_f("rex eos: %, total eos: % seos supply: % \n",
                asset(rex_eos_amount, EOS_SYMBOL),
                asset(eos_amount, EOS_SYMBOL), vault_supply);

        if (vault_supply.amount == 0) {
            return RATE_BASE;
        }

        return eos_amount * RATE_BASE / vault_supply.amount;
    }

    uint64_t get_rate(name contract, asset quantity) {

        auto balance = get_balance(contract, _self, quantity.symbol);
        balance += quantity;

        symbol_code issue_sc
            = symbol_code(string("S") + quantity.symbol.code().to_string());
        auto vault_supply = get_supply(STOKRN_ACCOUNT, issue_sc);

        // check(false, "balance " + balance.to_string() + " supply " + vault_supply.to_string());

        if (vault_supply.amount == 0) {
            return RATE_BASE;
        }

        return uint128_t(balance.amount) * RATE_BASE / vault_supply.amount;
    }

    s_collateral get_collateral(name contract, symbol sym, bool error_when_empty = true) {
        collaterals collateraltbl(_self, _self.value);
        auto        itr = collateraltbl.begin();
        while (itr != collateraltbl.end()) {
            if (itr->deposit_contract == contract && itr->deposit_symbol == sym) {
                break;
            }
            itr++;
        }

        check(!error_when_empty || itr != collateraltbl.end(),
              "deposit token not found");
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
        auto        itr = collateraltbl.begin();
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
        auto     now         = time_point_sec(current_time_point());
        auto     mr_itr      = rexbal_it->rex_maturities.begin();
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