/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */

#include "etbtoken.h"

namespace etb {

    const int64_t expire_time = 40 * 24 * 3600;//40天众筹期
    const double  ratio_max = 1.30;//倍数由1.30开始递减,第1天1.30,第2天1.29,...第29天1.01,第30天到第40天1.0
    const double  ratio_min = 1.00;
    const int64_t days__per_year = 365;
    const int64_t second_per_day = 24 * 3600;//每天解冻一点
    const int64_t reward_partner_base = 50000000;//奖励基线,为他人众筹超过5000EOS才有奖励
    const double  reward_partner_ratio = 0.05;//奖励代投伙伴5%
    const double  reward_team_ratio = 0.1;//奖励开发团队和社区10%
    #define ETB_SYMBOL S(4,ETB)

    void etbtoken::create()
    {
            require_auth( _self );

            auto sym = symbol_type(ETB_SYMBOL);

            stats statstable( _self, sym.name() );
            auto existing = statstable.find( sym.name() );
            eosio_assert( existing == statstable.end(), "token with symbol already exists" );

            statstable.emplace( _self, [&]( auto& s ) {
                s.supply.symbol = sym;
                s.max_supply.symbol = sym;
                s.create_time    = now();
                s.exchange_expire  = s.create_time + expire_time;
            });
    }

    void etbtoken::transfer( account_name from,
                          account_name to,
                          asset        quantity,
                          string       memo )
    {
            eosio_assert( from != to, "cannot transfer to self" );
            require_auth( from );
            eosio_assert( is_account( to ), "to account does not exist");
            auto sym = quantity.symbol.name();
            stats statstable( _self, sym );
            const auto& st = statstable.get( sym );

            require_recipient( from );
            require_recipient( to );

            eosio_assert( quantity.is_valid(), "invalid quantity" );
            eosio_assert( quantity.amount > 0, "must transfer positive quantity" );
            eosio_assert( quantity.symbol == st.supply.symbol, "symbol precision mismatch" );
            eosio_assert( memo.size() <= 256, "memo has more than 256 bytes" );


            sub_balance( from, quantity );
            add_balance( to, quantity, from );
    }

    void etbtoken::sub_balance( account_name owner, asset value ) {
            accounts from_acnts( _self, owner );

            const auto& from = from_acnts.get( value.symbol.name(), "no balance object found" );
            eosio_assert( from.balance.amount >= value.amount, "overdrawn balance" );


            if( from.balance.amount == value.amount ) {
                    from_acnts.erase( from );
            } else {
                    from_acnts.modify( from, owner, [&]( auto& a ) {
                        a.balance -= value;
                    });
            }
    }

    void etbtoken::add_balance( account_name owner, asset value, account_name ram_payer )
    {
            accounts to_acnts( _self, owner );
            auto to = to_acnts.find( value.symbol.name() );
            if( to == to_acnts.end() ) {
                    to_acnts.emplace( ram_payer, [&]( auto& a ){
                        a.balance = value;
                    });
            } else {
                    to_acnts.modify( to, 0, [&]( auto& a ) {
                        a.balance += value;
                    });
            }
    }

    void etbtoken::issue( account_name from,
                   account_name to,
                   asset        quantity)
    {
//            print("from:",name{from}, "\n");
//            print("to：",name{to}, "\n");
//            quantity.print();
            require_auth( from );

            eosio_assert( quantity.symbol == S(4,EOS), "the symbol is not EOS");
            eosio_assert( quantity.is_valid(), "invalid quantity");
            eosio_assert( quantity.amount > 0, "quantity must be positive");
            eosio_assert( quantity.amount % 10000 == 0, "amount is not INT" );

            symbol_name sym_name = symbol_type(ETB_SYMBOL).name();

            stats statstable( _self, sym_name );
            auto existing = statstable.find( sym_name );
            eosio_assert( existing != statstable.end(), "ETB token does not exist, create token before exchange" );
            auto& st = *existing;
            asset max_supply = st.max_supply;

//            print("issue：",name{st.creater}, "\n");

            //是否超过失效期
            eosio_assert(now() <= st.exchange_expire, "exchange time is expired");

            //转出EOS到合约账户
            action(
                    permission_level{ from, N(active) },
                    N(eosio.token), N(transfer),
                    std::make_tuple(from, _self, quantity, std::string("send EOS"))
            ).send();

            //倍数由1.30开始递减,第1天1.30,第2天1.29,...第29天1.01,第30天之后恒定为1.0
            int64_t days = (now() - st.create_time) / second_per_day;
//            print("now:",now(),"\n");
//            print("issue time:",st.create_time, "\n");
//            print("days:",days, "\n");

            double ratio = ratio_max - double(days)/100;
            if(ratio <= ratio_min){
                ratio = ratio_min;
            }


            asset to_quant{int64_t(quantity.amount * ratio), ETB_SYMBOL};
//            print("\n","to_quant:");
//            to_quant.print();

            //接收者信息
            etbinfos owner_etbinfos(_self, _self);
            auto itr = owner_etbinfos.find( to );
            if( itr == owner_etbinfos.end() ) {
                owner_etbinfos.emplace( from, [&]( auto& a ){
                    a.name = to;
                    a.buyfor_self = asset{0, S(4,EOS)};
                    a.buyfor_other = asset{0, S(4,EOS)};
                    a.receive = to_quant;
                    a.reward = asset{0, ETB_SYMBOL};
                    a.claimedreward = asset{0, ETB_SYMBOL};
                });
            } else {
                owner_etbinfos.modify( itr, 0, [&]( auto& a ) {
                    a.receive.amount += to_quant.amount;
                });
            }

            //合伙人账户
            asset reward_partner{0,ETB_SYMBOL};
            itr = owner_etbinfos.find( from );
            if( itr == owner_etbinfos.end() ) {
                owner_etbinfos.emplace( from, [&]( auto& a ){
                    a.name = from;
                    a.receive = asset{0, ETB_SYMBOL};
                    a.claimedreward = asset{0, ETB_SYMBOL};
                    if(from != to){//合伙人超过5000EOS后有奖励
                        reward_partner.amount = quantity.amount>=reward_partner_base? int64_t(to_quant.amount*reward_partner_ratio):0;
//                        reward_partner.print();

                        a.buyfor_self = asset{0, S(4,EOS)};
                        a.buyfor_other = quantity;
                        a.reward = reward_partner;
                    }else{
                        a.buyfor_self = quantity;
                        a.buyfor_other = asset{0, S(4,EOS)};
                        a.reward = asset{0, ETB_SYMBOL};
                    }
                });
            } else {
                owner_etbinfos.modify( itr, 0, [&]( auto& a ) {
                    if(from != to){
                        if(itr->buyfor_other.amount >= reward_partner_base){
                            reward_partner.amount = int64_t(to_quant.amount * reward_partner_ratio);
                        }else if(itr->buyfor_other.amount + quantity.amount >= reward_partner_base){
                            reward_partner.amount = int64_t((itr->buyfor_other.amount + quantity.amount) * ratio * reward_partner_ratio);
                        }
//                        reward_partner.print();

                        a.buyfor_other.amount += quantity.amount;
                        a.reward.amount += reward_partner.amount;
                    }else{
                        a.buyfor_self.amount += quantity.amount;
                    }
                });
            }


            //团队社区账户
            asset reward_team{int64_t(to_quant.amount*reward_team_ratio),ETB_SYMBOL};
//            reward_team.print();

            itr = owner_etbinfos.find( _self );
            if( itr == owner_etbinfos.end() ) {
                owner_etbinfos.emplace( _self, [&]( auto& a ){
                    a.name = _self;
                    a.buyfor_self = asset{0, S(4,EOS)};
                    a.buyfor_other = asset{0, S(4,EOS)};
                    a.receive = asset{0, ETB_SYMBOL};
                    a.reward = reward_team;
                    a.claimedreward = asset{0, ETB_SYMBOL};
                });
            } else {
                owner_etbinfos.modify( itr, 0, [&]( auto& a ) {
                    a.reward.amount += reward_team.amount;
                });
            }

            max_supply.amount += to_quant.amount + reward_partner.amount + reward_team.amount;
            statstable.modify( existing, 0, [&]( auto& a) {
                a.max_supply = max_supply;
                a.supply += to_quant;
            });

            add_balance( from, to_quant, from );

            if( to != from ) {
                SEND_INLINE_ACTION( *this, transfer, {from,N(active)}, {from, to, to_quant, std::string("issue ETB")} );
            }
    }

    void etbtoken::claimrewards( const account_name& owner ){
//            print("owner:",name{owner}, "\n");
            require_auth( owner );

            symbol_name sym_name = symbol_type(ETB_SYMBOL).name();
            stats statstable( _self, sym_name );
            auto existing = statstable.find( sym_name );
            eosio_assert( existing != statstable.end(), "ETB token does not exist, create token before exchange" );

//            print("now:",now(),"\n");
//            print("issue expire:",existing->exchange_expire,"\n");
            eosio_assert(now() > existing->exchange_expire, "cannot claim rewards until the etbexchange is finished");

            etbinfos owner_etbinfos(_self, _self);
            auto itr = owner_etbinfos.find( owner );
            eosio_assert(itr != owner_etbinfos.end(), "account does not have rewards");
            eosio_assert(itr->reward.amount > 0, "claimrewards is zero");
            eosio_assert(itr->reward.amount > itr->claimedreward.amount, "claimedrewards have claimed");

            eosio_assert(now() - itr->last_claim_time > second_per_day,"already claimed rewards within past day");

            asset reward(0, ETB_SYMBOL);

            int64_t  days = (now() - existing->exchange_expire + second_per_day - 1)/second_per_day;
            if(days > days__per_year) days = days__per_year;
            reward.amount = itr->reward.amount * days / days__per_year;
            reward.amount -= itr->claimedreward.amount;

            eosio_assert(reward.amount > 0, "claimedrewards is zero,please wait for days");


            statstable.modify( existing, 0, [&]( auto& a) {
                a.supply += reward;
            });

            add_balance( owner, reward, owner );

            owner_etbinfos.modify( itr, 0, [&]( auto& a ) {
                a.claimedreward.amount += reward.amount;
                a.last_claim_time = now();
            });

    }



} /// namespace eosio

EOSIO_ABI( etb::etbtoken, (create)(issue)(transfer)(claimrewards) )



