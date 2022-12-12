#!/bin/bash

echo "compiling... [stoken.defi]"
cd contracts/stoken
blanc++ src/stoken.cpp -I ./include
shasum -a 256 stoken.wasm

echo "compiling... [vault.defi]"
cd ../vault
blanc++ src/vault.cpp -I ./include
shasum -a 256 vault.wasm
