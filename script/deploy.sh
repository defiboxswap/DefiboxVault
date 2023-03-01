#!/bin/bash

cleos set account permission stoken.defi active --add-code
cleos set account permission vault.defi active --add-code
cleos set account permission award.defi active --add-code vault.defi

cleos set contract vault.defi contracts/vault/build/vault
cleos set contract stoken.defi contracts/stoken/build/stoken