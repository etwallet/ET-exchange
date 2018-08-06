#pragma once

#include <eosiolib/asset.hpp>
#include <eosiolib/eosio.hpp>
#include <musl/upstream/include/bits/stdint.h>

namespace etb {
    using namespace eosio;
   using eosio::asset;
   using eosio::symbol_type;
   using boost::container::flat_map;
   typedef double real_type;

   /**
    *  Uses Bancor math to create a 50/50 relay between two asset types. The state of the
    *  bancor exchange is entirely contained within this struct. There are no external
    *  side effects associated with using this API.
    */
   struct exchange_state {
       uint64_t id;
       uint128_t idxkey;
       asset    supply;
      struct connector {
          account_name  contract;
         asset balance;
         double weight = .5;

         EOSLIB_SERIALIZE( connector, (contract)(balance)(weight) )
      };

      connector base;
      connector quote;
      account_name  exchange_account;
      uint64_t primary_key()const { return id;}
      uint128_t by_contract_sym()const {return  idxkey;}

      asset convert_to_exchange( connector& c, asset in ); 
      asset convert_from_exchange( connector& c, asset in );
      asset convert( asset from, symbol_type to );

      EOSLIB_SERIALIZE( exchange_state, (id)(idxkey)(supply)(base)(quote)(exchange_account))
   };

   typedef eosio::multi_index<N(markets), exchange_state,
           indexed_by< N(idxkey), const_mem_fun<exchange_state, uint128_t,  &exchange_state::by_contract_sym> >
   > markets;



} /// namespace eosiosystem
