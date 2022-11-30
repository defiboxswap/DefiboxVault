# Defibox-Vault

# Overview

Vault Protocol Introduction The Vault protocol is the first single-token lossless yield protocol launched by Defibox. Users can earn corresponding token income by depositing tokens. The assets can be flexibly deposited and withdrawn with open and transparent on chain data. Valut income mainly comes from Defibox protocol income, Yield+ rewards, BP income, REX income, etc. At the same time, in order to improve the utility, the protocol will issue a standard EOS token called sToken, which represents a deposit certificate. sToken can be used in multiple DeFi protocols to obtain more benefits

# Audits

- <a href="https://defibox.s3.ap-northeast-1.amazonaws.com/pdf/Certik+Smart+Contract+Security+Audit+Report+For+Vault.pdf"> Sentnl Audit</a> (2022-11)
- <a href="https://www.certik.com/projects/defibox"> Certik Audit</a>

## Contracts

| name                                                | description     |
| --------------------------------------------------- | --------------- |
| [vault.defi](https://bloks.io/account/vault.defi)   | Vault Contract  |
| [stoken.defi](https://bloks.io/account/stoken.defi) | Stoken Contract |

## Quickstart

### `USER`

```bash
# deposit
$ cleos push action eosio.token transfer '["tester1","vault.defi","1.0000 EOS",""]' -p tester1

# withdraw
$ cleos push action stoken.defi transfer '["tester1","vault.defi","1.0000 SEOS",""]' -p tester1

# immediate withdraw at maturity
$ cleos push action vault.defi release '["tester1"]' -p tester1
```

### `ADMIN`

```bash
# modify configs
$ cleos push action vault.defi updatestatus '[1, 1, 1]' -p admin.defi

# create collateral tokens
$ cleos push action vault.defi createcoll '["eosio.token", "4,EOS", "award.defi", "fees.defi", "0.1000 EOS", "10", "30", "5000"]' -p admin.defi

# modify collateral tokens
$ cleos push action vault.defi updatecoll '[1, "award.defi", "fees.defi", "10", "0.2000 EOS" "10", "30", "5000"]' -p admin.defi
```

### `ANYONE`

```bash
# transfer from collateral->income_account to vault.defi (every 10 minutes)
cleos push action vault.defi income '[]' -p tester1
```

### Viewing Table Information

```bash
cleos get table vault.defi vault.defi config
cleos get table vault.defi tester1 releases
cleos get table vault.defi vault.defi collaterals

cleos get table stoken.defi tester1 accounts
cleos get table stoken.defi SEOS stat
```

## Table of Content

- [TABLE `configs`](#table-configs)
- [TABLE `collaterals`](#table-collaterals)
- [TABLE `releases`](#table-releases)
- [ACTION `updatestatus`](#action-updatestatus)
- [ACTION `createcoll`](#action-createcoll)
- [ACTION `updatecoll`](#action-updatecoll)
- [ACTION `proxyto`](#action-proxyto)
- [ACTION `buyallrex`](#action-buyallrex)
- [ACTION `buyrex`](#action-buyrex)
- [ACTION `sellallrex`](#action-sellallrex)
- [ACTION `sellrex`](#action-sellrex)
- [ACTION `sellnext`](#action-sellnext)
- [ACTION `sellnext2`](#action-sellnext2)
- [ACTION `income`](#action-income)
- [ACTION `release`](#action-release)
- [ACTION `colupadtelog`](#action-colupadtelog)
- [ACTION `depositlog`](#action-depositlog)
- [ACTION `releaselog`](#action-releaselog)
- [ACTION `withdrawlog`](#action-withdrawlog)

## TABLE `configs`

### params

- `{uint64_t} last_income_time` - (primary key) token symbol
- `{uint8_t} transfer_status` - transfer status (`0: suspended 1: open`)
- `{uint8_t} deposit_status` - deposit status (`0: suspended 1: open`)
- `{uint8_t} withdraw_status` - withdraw status (`0: suspended 1: open`)
- `{uint64_t} log_id` - Save the latest id of the `releases` table

### example

```json
{
  "last_income_time": 1669710600,
  "transfer_status": 1,
  "deposit_status": 1,
  "withdraw_status": 1,
  "log_id": 22
}
```

## TABLE `collaterals`

### params

- `{uint64_t} id` - the collateral id
- `{name} deposit_contract` - collateral token contract
- `{symbl} deposit_symbol` - collateral token symbol
- `{symbl} issue_symbol` - the token to be issue
- `{asset} last_income` - last transfer from collateral->income account
- `{asset} total_income` - transfer total quantity from collateral->income account
- `{uint16_t} income_ratio` - Percentage of collateral->income_account transferred
- `{name} income_account` - used to store reward accounts
- `{asset} min_quantity` - Minimum deposit quantity
- `{name} fees_account` - receiving service charge Account
- `{uint16_t} release_fees` - withdraw service fee
- `{uint16_t} refund_ratio` - The proportion of the withdrawal fee returned to the collateral->income_account

### example

```json
{
  "id": 1,
  "deposit_contract": "eosio.token",
  "deposit_symbol": "4,EOS",
  "issue_symbol": "4,SEOS",
  "last_income": "1.3344 EOS",
  "total_income": "5208.1385 EOS",
  "income_ratio": 50,
  "income_account": "award.defi",
  "min_quantity": "0.1000 EOS",
  "fees_account": "vfees.defi",
  "release_fees": 0,
  "refund_ratio": 0
}
```

## TABLE `releases`

### params

- `{uint64_t} id` - primary key
- `{asset} quantity` - the amount that can be released
- `{uint64_t} rate` - ratio of exchange
- `{block_timestamp} time` - time when the collateral can be released

### example

```json
{
  "id": 17,
  "quantity": "1000.0000 SUSDT",
  "rate": 199578590,
  "time": "2022-12-03T10:13:07.000"
}
```

## ACTION `updatestatus`

> Modifying Global Status.

- **authority**: `admin.defi`

### params

- `{uint8_t} transfer_status` - transfer status (`0: suspended 1: open`)
- `{uint8_t} deposit_status` - deposit status (`0: suspended 1: open`)
- `{uint8_t} withdraw_status` - withdraw status (`0: suspended 1: open`)

### example

```bash
$ cleos push action vault.defi updatestatus '[1, 1, 1]' -p admin.defi
```

## ACTION `createcoll`

> Create the collateral.

- **authority**: `admin.defi`

### params

- `{name} contract` - collateral token contract
- `{name} sym` - collateral token symbol
- `{name} income_account` - used to store reward accounts
- `{name} fees_account` - receiving service charge Account
- `{asset} min_quantity` - minimum deposit quantity
- `{uint16_t} income_ratio` - percentage of collateral->income_account transferred(pips 100/10000 of 1%)
- `{uint16_t} release_fees` - percentage of withdraw service fee(pips 100/10000 of 1%)
- `{uint16_t} refund_ratio` - the proportion of the withdrawal fee returned to the collateral->income_account(pips 100/10000 of 1%)

### example

```bash
$ cleos push action vault.defi createcoll '["eosio.token", "4,EOS", "award.defi", "fees.defi", "0.1000 EOS", "10", "30", "5000"]' -p admin.defi
```

## ACTION `updatecoll`

> Modify the configuration of the collateral.

- **authority**: `admin.defi`

### params

- `{uint64_t} collateral_id` - the collateral id
- `{name} income_account` - used to store reward accounts
- `{name} fees_account` - receiving service charge Account
- `{asset} min_quantity` - minimum deposit quantity
- `{uint16_t} income_ratio` - percentage of collateral->income_account transferred(pips 100/10000 of 1%)
- `{uint16_t} release_fees` - percentage of withdraw service fee(pips 100/10000 of 1%)
- `{uint16_t} refund_ratio` - the proportion of the withdrawal fee returned to the collateral->income_account(pips 100/10000 of 1%)

### example

```bash
$ cleos push action vault.defi updatecoll '[1, "award.defi", "fees.defi", "10", "0.2000 EOS", "10", "30", "5000"]' -p admin.defi
```

## ACTION `proxyto`

> Vote for another agent.

- **authority**: `admin.defi`

### params

- `{name} proxy` - account that accept votes

### example

```bash
$ cleos push action vault.defi proxyto '["voteto"]' -p admin.defi
```

## ACTION `buyallrex`

> Swap all the collateral for rex.

- **authority**: `get_self()` or `admin.defi`

```bash
$ cleos push action vault.defi buyallrex '[]' -p admin.defi
```

## ACTION `buyrex`

> Swap {{quantity}} the collateral for rex.

- **authority**: `get_self()` or `admin.defi`

### params

- `{asset} quantity` - oracle account

### example

```bash
$ cleos push action vault.defi buyrex '["10.0000 EOS"]' -p admin.defi
```

## ACTION `sellallrex`

> Sell all the Rexes.

- **authority**: `admin.defi`

```bash
$ cleos push action vault.defi sellallrex '[]' -p admin.defi
```

## ACTION `sellrex`

> Sell {{quantity}} of rex.

- **authority**: `admin.defi`

### params

- `{asset} quantity` - amount of rex to sell

### example

```bash
$ cleos push action vault.defi sellrex '["10.0000 EOS"]' -p admin.defi
```

## ACTION `sellnext`

> Sell {{quantity}} of rex for {{owner}}.

- **authority**: `get_self()`

### params

- `{name} owner` - the deposit account
- `{asset} quantity` - amount of rex to sell
- `{string} memo` - the memo string to accompany the transaction.

### example

```bash
$ cleos push action vault.defi sellnext '["depositowner", "10.0000 EOS", ""]' -p vault.defi
```

## ACTION `sellnext2`

> Sell {{quantity}} of rex for {{owner}}.

- **authority**: `get_self()`

### params

- `{name} owner` - the deposit account
- `{name} quantity` - amount of rex to sell
- `{name} memo` - the memo string to accompany the transaction.

### example

```bash
$ cleos push action vault.defi sellnext2 '["depositowner", "10.0000 EOS", ""]' -p vault.defi
```

## ACTION `income`

> Transfer from collateral->income_account to vault contract (every 10 minutes).

- **authority**: `anyone`

```bash
$ cleos push action vault.defi income '[]' -p any
```

## ACTION `release`

> The mortgaged property to be withdrawn and deposited after maturity.

- **authority**: `owner`

### params

- `{name} owner` - the deposit account

### example

```bash
$ cleos push action vault.defi release '[mydeposit]' -p mydeposit
```

## ACTION `colupadtelog`

> Generates a log when an collateral is created or modified.

- **authority**: `get_self()`

### params

- `{uint64_t} collateral_id` - the collateral id
- `{name} deposit_contract` - collateral token contract
- `{symbl} deposit_symbol` - collateral token symbol
- `{symbl} issue_symbol` - the token to be issue
- `{name} income_account` - used to store reward accounts
- `{name} fees_account` - receiving service charge Account
- `{asset} min_quantity` - Minimum deposit quantity
- `{uint16_t} income_ratio` - Percentage of collateral->income_account transferred
- `{uint16_t} release_fees` - withdraw service fee
- `{uint16_t} refund_ratio` - The proportion of the withdrawal fee returned to the collateral->income_account

### example

```json
{
  "collateral_id": "2",
  "deposit_contract": "tethertether",
  "deposit_symbol": "4,USDT",
  "issue_symbol": "4,SUSDT",
  "income_account": "award.defi",
  "fees_account": "fees.defi",
  "min_quantity": "0.1000 USDT",
  "income_ratio": 30,
  "release_fees": 30,
  "refund_ratio": 500
}
```

## ACTION `depositlog`

> Generates a log each time deposit.

- **authority**: `get_self()`

### params

- `{uint64_t} collateral_id` - the collateral id
- `{name} owner` - the deposit acount
- `{asset} quantity` - amount of deposit
- `{uint64_t} rate` - ratio of exchange
- `{block_timestamp} time` - the time of deposit

### example

```json
{
  "collateral_id": "2",
  "owner": "depositowner",
  "quantity": "100.0000 USDT",
  "rate": "199690010",
  "time": "2022-11-29T06:10:15.500"
}
```

## ACTION `releaselog`

> Generates a log when any user calls check_for_released.

- **authority**: `get_self()`

### params

- `{name} log_id` - table `releases` primary key
- `{uint64_t} collateral_id` - the collateral id
- `{name} owner` - the deposit acount
- `{asset} quantity` - amount of deposit
- `{uint64_t} rate` - ratio of exchange
- `{block_timestamp} time` - the time of release

### example

```json
{
  "log_id": "22",
  "collateral_id": "2",
  "owner": "depositowner",
  "quantity": "30.0000 SUSDT",
  "rate": "199690012",
  "time": "2022-12-04T06:14:52.500"
}
```

## ACTION `withdrawlog`

> Generates a log when the user draws the collateral.

- **authority**: `get_self()`

### params

- `{uint64_t} log_id` - table `releases` primary key
- `{uint64_t} collateral_id` - the collateral id
- `{name} owner` - the deposit acount
- `{asset} withdraw_quantity` - amount of withdraw
- `{asset} withdraw_award_fees` - amount of the withdrawal fee returned to the collateral->income_account
- `{asset} withdraw_sys_fees` - amount of the withdrawal fee to the collateral->fees_account
- `{asset} refund_award_quantity` - oracle account
- `{asset} refund_sys_quantity` - oracle account
- `{block_timestamp} time` - the block timestamp of the transaction

### example

```json
{
  "log_id": "13",
  "collateral_id": "1",
  "owner": "depositowner",
  "withdraw_quantity": "4000016.8402 EOS",
  "withdraw_award_fees": "0.0000 EOS",
  "withdraw_sys_fees": "0.0000 EOS",
  "refund_award_quantity": "0.0000 EOS",
  "refund_sys_quantity": "2449.4308 EOS",
  "time": "2022-11-29T02:35:03.000"
}
```
