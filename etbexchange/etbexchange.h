//
// Created by root1 on 18-7-26.
//

#ifndef EOSIO_ETBEXCHANGE_H
#define EOSIO_ETBEXCHANGE_H

#pragma once


#include <eosio.system/native.hpp>
#include <eosiolib/asset.hpp>
#include <eosiolib/time.hpp>
#include <eosiolib/privileged.hpp>
#include <eosiolib/singleton.hpp>
#include <eosiolib/eosio.hpp>
#include "exchange_state.hpp"
#include <string>


namespace etb {
    using namespace eosio;
    using std::string;
    using eosio::asset;
    using eosio::symbol_type;

    typedef double real_type;

    class exchange : public contract  {

    public:
        exchange(account_name self) : contract(self) {}

        void create(account_name payer, account_name exchange_account, asset eos_supply, account_name  token_contract,  asset token_supply);

        void buytoken(account_name payer, asset eos_quant,account_name token_contract, symbol_type token_symbol, account_name fee_account,int64_t fee_rate);

        void selltoken(account_name receiver,account_name token_contract, asset quant ,account_name fee_account,int64_t fee_rate);

        void addtoken( account_name account,asset quant,account_name token_contract, symbol_type token_symbol );

        void subtoken( account_name account,asset quant,account_name token_contract, symbol_type token_symbol );

        void setparam(account_name token_contract,symbol_type token_symbol, string paramname, string param);

        void pause();

        void restart();


    private:
        asset calcfee(asset quant, uint64_t fee_rate);

        void statsfee(exchange_state &market, asset eos_fee, asset token_fee);

        string to_string(asset quant);
    };

}
#endif //EOSIO_ETBEXCHANGE_H
