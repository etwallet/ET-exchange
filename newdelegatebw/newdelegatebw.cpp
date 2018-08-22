//
// Created by root1 on 18-8-21.
//

#include <eosiolib/eosio.hpp>
#include <eosiolib/asset.hpp>


using namespace eosio;

class newdelegatebw : public eosio::contract {
public:
    using contract::contract;

    /// @abi action
    void delegatebw(  account_name receiver,
                     asset stake_net_quantity,
                     asset stake_cpu_quantity ) {
        require_auth( N(delegatebwer) );

        action(//给推荐人转入EOS
                  permission_level{ _self, N(active) },
                  N(eosio), N(delegatebw),
                  std::make_tuple(_self, receiver,stake_net_quantity, stake_cpu_quantity, false)
          ).send();
    }

    void undelegatebw(  account_name receiver,
                      asset stake_net_quantity,
                      asset stake_cpu_quantity ) {
        require_auth( N(delegatebwer) );

        action(//给推荐人转入EOS
                permission_level{ _self, N(active) },
                N(eosio), N(undelegatebw),
                std::make_tuple(_self, receiver,stake_net_quantity, stake_cpu_quantity)
        ).send();
    }
};

EOSIO_ABI( newdelegatebw, (delegatebw)(undelegatebw) )
