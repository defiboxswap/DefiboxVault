export interface KV {
  key: string;
  value: string;
}

export interface Extendedstring {
  quantity: string;
  contract: string;
}

export interface ExtendedSymbol {
  sym: string;
  contract: string;
}

export interface Release {
  id: number;
  quantity: string;
  rate: number;
  time: string;
}
export interface Collateral {
  id: number;
  deposit_contract: string;
  deposit_symbol: string;
  issue_symbol: string;
  last_income: string;
  total_income: string;
  income_ratio: number;
  income_account: string;
  min_quantity: string;
  fees_account: string;
  release_fees: number;
  refund_ratio: number;
}
export interface Config {
  last_income_time: number;
  transfer_status: number;
  deposit_status: number;
  withdraw_status: number;
  log_id: number;
}