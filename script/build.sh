#!/bin/bash

PROJECT_HOME=`pwd`

## stoken build
mkdir $PROJECT_HOME/contracts/stoken/build
cd $PROJECT_HOME/contracts/stoken/build
cmake ..
make

## vault build
mkdir $PROJECT_HOME/contracts/vault/build
cd $PROJECT_HOME/contracts/vault/build
cmake ..
make

# sha256sum
shasum -a 256 $PROJECT_HOME/contracts/stoken/build/stoken/stoken.wasm
shasum -a 256 $PROJECT_HOME/contracts/vault/build/vault/vault.wasm