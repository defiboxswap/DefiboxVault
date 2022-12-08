import { Name, Asset, TimePointSec } from "@greymass/eosio";
import { Account } from "@proton/vert"

import { expectToThrow } from "@tests/helpers";
import { Collateral, Release, Config } from "@tests/interfaces";
import { contracts, blockchain, award_account } from "@tests/init";
import { RATE_BASE, INCOME_PERIOD_INTERVAL, RATIO_MULTIPER } from "@tests/constants";
import { sub, add, muldiv, randomInt, randomFloat } from "@tests/helpers";

const VAULT_SCOPE = Name.from('vault.defi').value.value;

const getColl = (id: number): Collateral => {
  return contracts.vault.tables.collaterals(VAULT_SCOPE).getTableRow(BigInt(id));
}

const getBalance = (account: Name | string, contract: Account, symcode: string): number => {
  let scope;
  if (typeof account === "string") {
    scope = Name.from(account).value.value;
  } else {
    scope = account.value.value;
  }
  const primaryKey = Asset.SymbolCode.from(symcode).value.value;
  const result = contract.tables.accounts(scope).getTableRow(primaryKey);
  if (result?.balance) { return Asset.from(result.balance).value; }
  return 0;
}

const getStat = (contract: Account, symcode: string): number => {
  const primaryKey = Asset.SymbolCode.from(symcode).value.value;
  const result = contract.tables.stat(primaryKey).getTableRows()[0];
  if (result?.supply) { return Asset.from(result.supply).value; }
  return 0;
}

const getReleases = (owner: string): Release[] => {
  const scope = Name.from(owner).value.value;
  return contracts.vault.tables.releases(scope).getTableRows();
}

const getConfig = (): Config => {
  return contracts.vault.tables.config(VAULT_SCOPE).getTableRows()[0];
}

const getIncomeAmount = (amount: number, income_ratio: number, duration: number, decimal: number): number => {
  let ratio = muldiv(income_ratio, duration, INCOME_PERIOD_INTERVAL, 0);
  if (ratio > RATIO_MULTIPER) {
    ratio = RATIO_MULTIPER;
  }
  return muldiv(amount, ratio, RATIO_MULTIPER, decimal);
}

const getRate = (contract: Account, symbl: string, amount: number): number => {
  const deposit_balance = getBalance(contracts.vault.name, contract, symbl);
  const stoken_supply = getStat(contracts.stoken, `S${symbl}`)
  let rate = RATE_BASE;
  if (stoken_supply > 0) {
    rate = muldiv(add(deposit_balance, amount), RATE_BASE, stoken_supply, 0);
  }
  return rate;
}

const getIssueAmount = (deposit_amount: number, contract: Account, symbl: string, decimal: number): number => {
  return muldiv(deposit_amount, RATE_BASE, getRate(contract, symbl, 0), decimal);
}

describe('vault.defi', () => {
  it("missing required authority admin.defi", async () => {
    let action = contracts.vault.actions.updatestatus([1, 1, 1]).send();
    await expectToThrow(action, "missing required authority admin.defi");

    action = contracts.vault.actions.createcoll({ "contract": "tethertether", "sym": "4,USDT", "income_ratio": 50, "income_account": "award.defi", "min_quantity": "0.1000 USDT", "fees_account": "vfees.defi", "release_fees": 30, "refund_ratio": 5000 }).send();
    await expectToThrow(action, "missing required authority admin.defi");

    action = contracts.vault.actions.updatecoll({ "collateral_id": 1, "income_ratio": 50, "income_account": "award.defi", "min_quantity": "0.1000 USDT", "fees_account": "vfees.defi", "release_fees": 30, "refund_ratio": 5000 }).send();
    await expectToThrow(action, "missing required authority admin.defi");

    action = contracts.vault.actions.proxyto(["account1"]).send();
    await expectToThrow(action, "missing required authority admin.defi");
  });

  it("config::updatestatus", async () => {
    await contracts.vault.actions.updatestatus([1, 1, 1]).send("admin.defi@active");
    const config = getConfig();
    expect(config.transfer_status).toEqual(1);
    expect(config.deposit_status).toEqual(1);
    expect(config.withdraw_status).toEqual(1);
  });

  it("collateral::createcoll", async () => {
    const collateral = {
      "contract": "tethertether",
      "sym": "4,USDT",
      "income_ratio": 50,
      "income_account": "award.defi",
      "min_quantity": "0.1000 USDT",
      "fees_account": "vfees.defi",
      "release_fees": 30,
      "refund_ratio": 5000
    };
    await contracts.vault.actions.createcoll(collateral).send("admin.defi@active");
    const coll = getColl(1);
    expect(coll).toEqual({
      "id": 1,
      "deposit_contract": "tethertether",
      "deposit_symbol": "4,USDT",
      "issue_symbol": "4,SUSDT",
      "last_income": "0.0000 USDT",
      "total_income": "0.0000 USDT",
      "income_ratio": 50,
      "income_account": "award.defi",
      "min_quantity": "0.1000 USDT",
      "fees_account": "vfees.defi",
      "release_fees": 30,
      "refund_ratio": 5000
    });
  });

  it("collateral::updatecoll", async () => {
    const update_row = {
      "collateral_id": 1,
      "income_account": "award.defi",
      "fees_account": "vfees.defi",
      "min_quantity": "10.0000 USDT",
      "income_ratio": 60,
      "release_fees": 20,
      "refund_ratio": 3000
    };
    await contracts.vault.actions.updatecoll(update_row).send("admin.defi@active");
    expect(getColl(update_row.collateral_id))
      .toEqual({
        "id": 1,
        "deposit_contract": "tethertether",
        "deposit_symbol": "4,USDT",
        "issue_symbol": "4,SUSDT",
        "last_income": "0.0000 USDT",
        "total_income": "0.0000 USDT",
        "income_ratio": 60,
        "income_account": "award.defi",
        "min_quantity": "10.0000 USDT",
        "fees_account": "vfees.defi",
        "release_fees": 20,
        "refund_ratio": 3000
      });
  });

  it("collateral::income", async () => {
    // first set last_income_time
    await contracts.vault.actions.income().send();

    const before_coll = getColl(1);

    const deposit_contract = blockchain.getAccount(Name.from(before_coll.deposit_contract)) as Account;
    const deposit_symbol = Asset.Symbol.from(before_coll.deposit_symbol);

    // income account blance
    const award_balance = getBalance(award_account.name, deposit_contract, deposit_symbol.name);

    const before_config = getConfig();
    const before_vault_balance = getBalance(contracts.vault.name, deposit_contract, deposit_symbol.name);

    // income reward
    blockchain.addTime(TimePointSec.from(randomInt(600, 3600)));
    await contracts.vault.actions.income().send();

    const after_coll = getColl(1);
    const after_config = getConfig();
    const after_vault_balance = getBalance(contracts.vault.name, deposit_contract, deposit_symbol.name);

    const period = sub(after_config.last_income_time, before_config.last_income_time);
    const income_ratio = before_coll.income_ratio;
    const income_amount = getIncomeAmount(award_balance, income_ratio, period, deposit_symbol.precision);

    expect(Asset.from(after_coll.last_income).value).toBe(income_amount);
    expect(sub(Asset.from(after_coll.total_income).value, Asset.from(before_coll.total_income).value)).toBe(income_amount);
    expect(sub(after_vault_balance, before_vault_balance)).toBe(income_amount);
  });

  it("collateral::deposit", async () => {
    const coll = getColl(1);
    const deposit_contract = blockchain.getAccount(Name.from(coll.deposit_contract)) as Account;
    const deposit_symbol = Asset.Symbol.from(coll.deposit_symbol);
    const issue_symbol = Asset.Symbol.from(coll.issue_symbol);

    const transfer_action = {
      "from": "account1",
      "to": "vault.defi",
      "quantity": `1000.0000 ${deposit_symbol.name}`,
      "memo": ""
    }
    // deposit
    const issue_amount = getIssueAmount(Asset.from(transfer_action.quantity).value, deposit_contract, deposit_symbol.name, issue_symbol.precision);
    const before_vault_balance = getBalance(contracts.vault.name, deposit_contract, deposit_symbol.name);

    await deposit_contract.actions.transfer(transfer_action).send("account1@active");
    const after_vault_balance = getBalance(contracts.vault.name, deposit_contract, deposit_symbol.name);

    expect(sub(after_vault_balance, before_vault_balance)).toEqual(Asset.from(transfer_action.quantity).value);
    expect(getBalance("account1", contracts.stoken, issue_symbol.name))
      .toBe(issue_amount);

    // income reward
    blockchain.addTime(TimePointSec.from(randomInt(600, 3600)));
    await contracts.vault.actions.income().send();

    const transfer_action1 = {
      "from": "account1",
      "to": "vault.defi",
      "quantity": `900.0000 ${deposit_symbol.name}`,
      "memo": ""
    }
    // deposit
    const issue_amount1 = getIssueAmount(Asset.from(transfer_action1.quantity).value, deposit_contract, deposit_symbol.name, issue_symbol.precision);
    const before_susdt_balance = getBalance("account1", contracts.stoken, issue_symbol.name)
    const before_usdt_balance = getBalance("vault.defi", deposit_contract, deposit_symbol.name)
    await deposit_contract.actions.transfer(transfer_action1).send("account1@active");
    const after_susdt_balance = getBalance("account1", contracts.stoken, issue_symbol.name)
    const after_usdt_balance = getBalance("vault.defi", deposit_contract, deposit_symbol.name);

    expect(sub(after_usdt_balance, before_usdt_balance)).toBe(Asset.from(transfer_action1.quantity).value);
    expect(sub(after_susdt_balance, before_susdt_balance)).toBe(issue_amount1);
  });

  it("collateral::withdraw", async () => {
    const coll = getColl(1);
    const deposit_contract = blockchain.getAccount(Name.from(coll.deposit_contract)) as Account;
    const deposit_symbol = Asset.Symbol.from(coll.deposit_symbol);
    const issue_symbol = Asset.Symbol.from(coll.issue_symbol);

    const susdt_amount = getBalance("account1", contracts.stoken, issue_symbol.name);
    const rate = getRate(deposit_contract, `${deposit_symbol.name}`, 0);
    // The remaining token of the user
    let left_mount = susdt_amount;
    // 100 time
    const times = 100;
    for (let index = 0; index < times; index++) {
      const amount = Asset.from(randomFloat(1, muldiv(1, susdt_amount, times, issue_symbol.precision)), issue_symbol);
      const transfer_action = {
        "from": "account1",
        "to": "vault.defi",
        "quantity": amount,
        "memo": ""
      }
      // lock 5 days
      await contracts.stoken.actions.transfer(transfer_action).send("account1@active");
      const releases = getReleases("account1");
      const release = releases[releases.length - 1];

      left_mount = sub(left_mount, amount.value);

      expect(Asset.from(release.quantity).value).toBe(amount.value);
      expect(Number(release.rate)).toBe(rate);
      expect(getBalance("account1", contracts.stoken, issue_symbol.name)).toBe(left_mount);
    }
  });

  it("collateral::release", async () => {
    // add 10 days
    blockchain.addTime(TimePointSec.from(10 * 86400));
    // income reward
    await contracts.vault.actions.income().send();

    const coll = getColl(1);
    const deposit_contract = blockchain.getAccount(Name.from(coll.deposit_contract)) as Account;
    const deposit_symbol = Asset.Symbol.from(coll.deposit_symbol);

    const releases = getReleases("account1");
    for (const release of releases) {

      const new_rate = getRate(deposit_contract, `${deposit_symbol.name}`, 0);

      const before_account1_balance = getBalance("account1", deposit_contract, deposit_symbol.name)
      const before_income_account_balance = getBalance(coll.income_account, deposit_contract, deposit_symbol.name)
      const before_fee_account_balance = getBalance(coll.fees_account, deposit_contract, deposit_symbol.name)
      // release
      await contracts.vault.actions.release(["account1"]).send("account1@active");
      const after_account1_balance = getBalance("account1", deposit_contract, deposit_symbol.name);
      const after_income_account_balance = getBalance(coll.income_account, deposit_contract, deposit_symbol.name);
      const after_fee_account_balance = getBalance(coll.fees_account, deposit_contract, deposit_symbol.name);
      // owner get/ fee_amount
      const release_amount = muldiv(Asset.from(release.quantity).value, release.rate, RATE_BASE, deposit_symbol.precision);
      const release_fee = muldiv(release_amount, coll.release_fees, RATIO_MULTIPER, deposit_symbol.precision);
      const withdraw_amount = sub(release_amount, release_fee);

      const new_ratio_release_amount = muldiv(Asset.from(release.quantity).value, new_rate, RATE_BASE, deposit_symbol.precision);
      const extra_rewards = sub(new_ratio_release_amount, release_amount);

      // income_account,fees_account get
      const income_account_refund =
        add(
          muldiv(release_fee, coll.refund_ratio, RATIO_MULTIPER, deposit_symbol.precision),
          muldiv(extra_rewards, coll.refund_ratio, RATIO_MULTIPER, deposit_symbol.precision),
        );
      const fee_account_income = sub(add(release_fee, extra_rewards), income_account_refund);

      expect(sub(after_account1_balance, before_account1_balance)).toBe(withdraw_amount);
      expect(sub(after_fee_account_balance, before_fee_account_balance)).toBe(fee_account_income);
      expect(sub(after_income_account_balance, before_income_account_balance)).toBe(income_account_refund);
    }
    expect(getReleases("account1").length).toBe(0);

  });
});
