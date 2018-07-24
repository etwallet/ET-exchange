//
// Created by root1 on 18-7-19.
//

#ifndef EOSIO_ICO_H
#define EOSIO_ICO_H


/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#pragma once

#include <eosiolib/asset.hpp>
#include <eosiolib/eosio.hpp>

#include <string>


namespace etb {

    using namespace eosio;
    using std::string;


    class etbexchange : public contract {
    public:
        etbexchange( account_name self ):contract(self){}

        void create();

        void issue( account_name from,
                    account_name to,
                    asset        quantity);

        void transfer( account_name from,
                       account_name to,
                       asset        quantity,
                       string       memo );

        void claimrewards( const account_name& owner );


    private:
        struct account {
            asset    balance;

            uint64_t primary_key()const { return balance.symbol.name(); }
        };

        struct currency_stats {
            asset          supply;
            asset          max_supply;
//            account_name   creater;
            uint64_t       create_time;
            uint64_t       exchange_expire;
            uint64_t primary_key()const { return supply.symbol.name(); }
        };

        struct exchange_table{
            account_name   name;
            asset  exchange_self;
            asset  exchange_other;
            asset  receive;
            asset  reward;
            asset  claimedreward;
            uint64_t last_claim_time;
            uint64_t primary_key()const { return name; }
        };

        typedef eosio::multi_index<N(accounts), account> accounts;
        typedef eosio::multi_index<N(stat), currency_stats> stats;
        typedef eosio::multi_index<N(exchanges), exchange_table> exchanges;

//        void issue( account_name to, asset quantity, string memo );
        void sub_balance( account_name owner, asset value );
        void add_balance( account_name owner, asset value, account_name ram_payer );

    public:
        struct transfer_args {
            account_name  from;
            account_name  to;
            asset         quantity;
            string        memo;
        };
    };


} /// namespace etb




#endif //EOSIO_ICO_H
