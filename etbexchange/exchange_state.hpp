#pragma once

#include <eosiolib/asset.hpp>
#include <eosiolib/eosio.hpp>
#include <musl/upstream/include/bits/stdint.h>
#include <eosiolib/time.hpp>

namespace etb {
    using namespace eosio;
    using namespace std;
    using eosio::asset;
    using eosio::symbol_type;
    typedef double real_type;

    struct earnings{
        asset eos_fee;
        asset token_fee;
    };
    struct connector {
        account_name contract;
        asset balance;
        double weight = .5;
    };
   /**
    *  Uses Bancor math to create a 50/50 relay between two asset types. The state of the
    *  bancor exchange is entirely contained within this struct. There are no external
    *  side effects associated with using this API.
    */
   struct exchange_state {
       uint64_t id;
       uint128_t idxkey;
       asset supply;

       connector base;                      //交易池token实时信息
       connector quote;                     //交易池eos实时信息
       account_name exchange_account;       //交易池账号
       uint64_t buy_fee_rate;              //手续费率:[0,10000),如:50等同于万分之50; 0等同于无手续费
       uint64_t sell_fee_rate;             //手续费率:[0,10000),如:50等同于万分之50; 0等同于无手续费

       time_point_sec date;       //当天时间
       earnings total_fee;         //总收益
       earnings today_fee;         //当天收益
       earnings yesterday_fee;     //昨日收益

       bool support_banker;               //支持参股增加代币
       uint64_t banker_max_number=100;        //支持参股的最大数量

       uint64_t primary_key() const { return id; }

       uint128_t by_contract_sym() const { return idxkey; }

       uint64_t  by_contract() const{return base.contract;};

       asset convert_to_exchange(connector &c, asset in);

       asset convert_from_exchange(connector &c, asset in);

       asset convert( asset from, symbol_type to);

   };

   typedef eosio::multi_index<N(markets), exchange_state,
           indexed_by< N(idxkey), const_mem_fun<exchange_state, uint128_t,  &exchange_state::by_contract_sym>>,
           indexed_by< N(thirdkey), const_mem_fun<exchange_state, uint64_t,  &exchange_state::by_contract>>
   > markets;


//    struct exchange_state1 {
//        uint64_t id;
//        uint128_t idxkey;
//        asset    supply;
//
//        connector base;
//        connector quote;
//        account_name  exchange_account;
//        uint64_t buy_fee_rate;              //手续费率:[0,10000),如:50等同于万分之50; 0等同于无手续费
//        uint64_t sell_fee_rate;             //手续费率:[0,10000),如:50等同于万分之50; 0等同于无手续费
//        time_point_sec date;       //当天时间
//        earnings total_fee;         //总收益
//        earnings today_fee;         //当天收益
//        earnings yesterday_fee;     //昨日收益
//        bool support_addtoken;   //支持参股增加代币
//        uint64_t addtoken_max_number=100;        //支持参股的最大数量
//
//        uint64_t primary_key()const { return id;}
//    };
//
//    typedef eosio::multi_index<N(markets1), exchange_state1> markets1;

    struct holderinfo{
        asset eos_in;               //投入的eos量
        asset token_in;             //投入的token量
        asset eos_holding;          //当前持有的eos量,所有的eos_holding总和等于total_quant
        asset token_holding;        //当前持有的token量
    };
    //股东持有人信息
    struct shareholder{
        uint64_t id;
        uint128_t idxkey;

        asset total_quant;                                  //总EOS量
        std::map<account_name, holderinfo> map_acc_info;    //account_name account~holderinfo;股东账户名～资金量

        uint64_t primary_key() const { return id; }

        uint128_t by_contract_sym() const { return idxkey; }

        uint64_t  by_contract() const{return uint64_t(idxkey>>64);};

    };
    typedef eosio::multi_index<N(shareholders), shareholder,
            indexed_by< N(idxkey), const_mem_fun<shareholder, uint128_t,  &shareholder::by_contract_sym>>,
            indexed_by< N(thirdkey), const_mem_fun<shareholder, uint64_t,  &shareholder::by_contract>>
    > shareholders;


} /// namespace eosiosystem
