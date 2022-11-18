# Defibox-Vault

# Overview

Vault Protocol Introduction The Vault protocol is the first single-token lossless yield protocol launched by Defibox. Users can earn corresponding token income by depositing tokens. The assets can be flexibly deposited and withdrawn with open and transparent on chain data. Valut income mainly comes from Defibox protocol income, Yield+ rewards, BP income, REX income, etc. At the same time, in order to improve the utility, the protocol will issue a standard EOS token called sToken, which represents a deposit certificate. sToken can be used in multiple DeFi protocols to obtain more benefits

# Audits

- <a href="https://defibox.s3.ap-northeast-1.amazonaws.com/pdf/Certik+Smart+Contract+Security+Audit+Report+For+Vault.pdf"> Sentnl Audit</a> (2022-11)

## Contracts

| name  | description |
|-------|-------------|
| [vault.defi](https://bloks.io/account/vault.defi) | Vault Contract |
| [stoken.defi](https://bloks.io/account/stoken.defi) | Stoken Contract |


## Actions

### Create collateral tokens
```bash
cleos push action vault.defi createcoll '["eosio.token", "4,EOS", "award.defi", "fees.defi", "0.1000 EOS", "10", "30", "5000"]' -p admin.defi
cleos push action vault.defi createcoll '["tethertether", "4,USDT", "award.defi", "fees.defi", "0.1000 USDT", "10", "30", "5000"]' -p admin.defi
```

### Modify collateral tokens
```bash
cleos push action vault.defi updatecoll '[1, "award.defi", "fees.defi", "10", "0.2000 EOS"]' -p admin.defi
cleos push action vault.defi updatestatus '[1, 1, 1]' -p admin.defi
```

### Assign rewards to the vault pool every 10 minutes
```bash
cleos push action vault.defi income '[]' -p tester1
```
### deposit
```bash
cleos push action eosio.token transfer '["tester1","vault.defi","1.0000 EOS",""]' -p tester1
cleos push action tethertether transfer '["tester1","vault.defi","1.0000 USDT",""]' -p tester1
```

### withdraw
```bash
cleos push action stoken.defi transfer '["tester1","vault.defi","1.0000 SEOS",""]' -p tester1
cleos push action stoken.defi transfer '["tester1","vault.defi","1.0000 SUSDT",""]' -p tester1
```

### Immediate withdraw at maturity
```bash
cleos push action vault.defi release '["tester1"]' -p tester1
```

### Buy all rex
```bash
cleos push action vault.defi buyallrex '[]' -p admin.defi
```
### Sell all rex
```bash
cleos push action vault.defi sellallrex '[]' -p admin.defi
```

### Viewing Table Information
```bash
cleos get table vault.defi vault.defi config
cleos get table vault.defi tester1 releases
cleos get table vault.defi vault.defi collaterals

cleos get table stoken.defi tester1 accounts
cleos get table stoken.defi SEOS stat
```