import { Blockchain, AccountPermission } from "@proton/vert"
import { TimePointSec, Authority, PermissionLevel } from "@greymass/eosio";

export const blockchain = new Blockchain()

// contracts
export const contracts = {
  stoken: blockchain.createContract('stoken.defi', 'contracts/stoken/stoken', true),
  vault: blockchain.createContract('vault.defi', 'contracts/vault/vault', true),
  USDT: blockchain.createContract('tethertether', 'tests/eosio/eosio.token'),
}

// accounts
export const accounts = blockchain.createAccounts("tokens", "eosio.rex", 'admin.defi', "vfees.defi", 'account1', 'account2', "account3", "account5", "account6");

export const award_account = blockchain.createAccount("award.defi");
award_account.setPermissions([AccountPermission.from({
  parent: "owner",
  perm_name: "active",
  required_auth: Authority.from({
    threshold: 1,
    accounts: [{
      weight: 1,
      permission: PermissionLevel.from("award.defi@eosio.code")
    },
    {
      weight: 1,
      permission: PermissionLevel.from("vault.defi@eosio.code")
    }]
  })
}),
]);
accounts.push(award_account);

// permission
contracts.vault.setPermissions([AccountPermission.from({
  parent: "owner",
  perm_name: "active",
  required_auth: Authority.from({
    threshold: 1,
    accounts: [{
      weight: 1,
      permission: PermissionLevel.from("stoken.defi@eosio.code")
    },
    {
      weight: 1,
      permission: PermissionLevel.from("vault.defi@eosio.code")
    }]
  })
})]);


// one-time setup
beforeAll(async () => {
  blockchain.setTime(TimePointSec.from(new Date()));

  // create USDT token
  await contracts.USDT.actions.create(["tethertether", "10000000000.0000 USDT"]).send("tethertether@active");
  await contracts.USDT.actions.issue(["tethertether", "10000000000.0000 USDT", "init"]).send("tethertether@active");
  await contracts.USDT.actions.transfer(["tethertether", "award.defi", "200000.0000 USDT", "init"]).send("tethertether@active");
  await contracts.USDT.actions.transfer(["tethertether", "account1", "100000.0000 USDT", "init"]).send("tethertether@active");
  await contracts.USDT.actions.transfer(["tethertether", "account2", "500000.0000 USDT", "init"]).send("tethertether@active");
  await contracts.USDT.actions.transfer(["tethertether", "account3", "100000.0000 USDT", "init"]).send("tethertether@active");
  await contracts.USDT.actions.transfer(["tethertether", "account5", "200000.0000 USDT", "init"]).send("tethertether@active");
  await contracts.USDT.actions.transfer(["tethertether", "account6", "200000.0000 USDT", "init"]).send("tethertether@active");
});