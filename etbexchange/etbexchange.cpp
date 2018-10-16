

#include "etbexchange.h"
#include <cmath>

namespace etb {
    using namespace eosio;
    const int64_t max_fee_rate = 10000;
    const int64_t day = 24 * 3600;
//    const int64_t day = 60;

    /*  创建代币bancor池
     *  payer:			    支付账号,从这个账号转出代币和EOS到bancor池账号
     *  exchange_account:	bancor池账号,TEST代币和EOS代币都在此账号名下:如:etbexchange1
     *  eos_supply:		    初始化EOS数量:如: 100000.0000 EOS
     *  token_contract: 	代币属于哪个合约,如:issuemytoken部署创建了TEST代币
     *  token_supply:		初始化代币数量:如:1000000.0000 TEST代币
    */
    void exchange::create(account_name payer, account_name exchange_account, asset eos_supply, account_name  token_contract,  asset token_supply)
    {
        require_auth( _self );

        eosio_assert(token_supply.amount > 0, "invalid token_supply amount");
        eosio_assert(token_supply.symbol.is_valid(), "invalid token_supply symbol");
        eosio_assert(token_supply.symbol != S(4,EOS), "token_supply symbol cannot be EOS");
        eosio_assert(eos_supply.amount > 0, "invalid eos_supply amount");
        eosio_assert(eos_supply.symbol == S(4,EOS), "eos_supply symbol only support EOS");

        markets _market(_self,_self);

        uint128_t idxkey = (uint128_t(token_contract) << 64) | token_supply.symbol.value;

        auto idx = _market.template get_index<N(idxkey)>();
        auto itr = idx.find(idxkey);

        eosio_assert(itr == idx.end(), "token market already created");

        action(//给交易所账户转入EOS
                permission_level{ payer, N(active) },
                N(eosio.token), N(transfer),
                std::make_tuple(payer, exchange_account, eos_supply, std::string("send EOS to exchange_account"))
        ).send();

        action(//给交易所账户转入token
                permission_level{ payer, N(active) },
                token_contract, N(transfer),
                std::make_tuple(payer, exchange_account, token_supply, std::string("send token to exchange_account"))
        ).send();

        auto pk = _market.available_primary_key();
        _market.emplace( _self, [&]( auto& m ) {
            m.id = pk;
            m.idxkey = idxkey;
            m.supply.amount = 100000000000000ll;
            m.supply.symbol = S(0,ETBCORE);
            m.base.contract = token_contract;
            m.base.balance.amount = token_supply.amount;
            m.base.balance.symbol = token_supply.symbol;
            m.quote.contract = N(eosio.token);
            m.quote.balance.amount = eos_supply.amount;
            m.quote.balance.symbol = eos_supply.symbol;

            m.date = time_point_sec(now()/day*day);
            m.total_fee.eos_fee = asset{0, eos_supply.symbol};
            m.total_fee.token_fee = asset{0, token_supply.symbol};
            m.today_fee.eos_fee = asset{0, eos_supply.symbol};
            m.today_fee.token_fee = asset{0, token_supply.symbol};
            m.yesterday_fee.eos_fee = asset{0, eos_supply.symbol};
            m.yesterday_fee.token_fee = asset{0, token_supply.symbol};

            m.exchange_account = exchange_account;
        });

        shareholders _shareholders(_self, _self);
        _shareholders.emplace( _self, [&]( auto& m ) {
            m.id = pk;
            m.idxkey = idxkey;
            m.total_quant = eos_supply;

            m.map_acc_info[payer].eos_in = eos_supply;
            m.map_acc_info[payer].token_in = token_supply;
            m.map_acc_info[payer].eos_holding = eos_supply;
            m.map_acc_info[payer].token_holding = token_supply;

        });
    }


    /*  购买代币,收取万分之markets.buy_fee_rate手续费,由交易池所有股东共享
     *  payer: 	            买币账号
     *  eos_quant:		    用quant个EOS购买代币
     *  token_contract: 	代币属于哪个合约,如TEST代币是issuemytoken部署创建的
     *  token_symbol:		想要购买的代币符号:如"4,TEST",4是发币的小数点位数
     *  fee_account:		收取手续费的账号,payer==fee_account相当于无手续费
     *  fee_rate:		    手续费率:[0,10000),如:50等同于万分之50; 0等同于无手续费
     * */
    void exchange::buytoken( account_name payer, asset eos_quant,account_name token_contract, symbol_type token_symbol, account_name fee_account,int64_t fee_rate){
        require_auth( payer );

        eosio_assert(eos_quant.amount > 0, "must purchase a positive amount" );
        eosio_assert(eos_quant.symbol == S(4, EOS), "eos_quant symbol must be EOS");
        eosio_assert(token_symbol.is_valid(), "invalid token_symbol");
        eosio_assert(fee_rate >= 0 && fee_rate < max_fee_rate, "invalid fee_rate, 0<= fee_rate < 10000");

        markets _market(_self,_self);

        uint128_t idxkey = (uint128_t(token_contract) << 64) | token_symbol.value;

        print("idxkey=",idxkey,",contract=",token_contract,",symbol=",token_symbol.value);

        auto idx = _market.template get_index<N(idxkey)>();
        auto market = idx.find(idxkey);
        eosio_assert(market!=idx.end(),"token market does not exist");

        auto quant_after_fee = eos_quant;

        if(fee_rate > 0 && payer != fee_account){
            auto fee = calcfee(eos_quant, fee_rate);
            quant_after_fee -= fee;

            //给手续费账户转手续费
            action(
                    permission_level{ payer, N(active) },
                    market->quote.contract, N(transfer),
                    std::make_tuple(payer, fee_account, fee, std::string("send EOS fee to fee_account:" + to_string(fee)))
            ).send();
        }

        eosio_assert( quant_after_fee.amount > 0, "quant_after_fee must a positive amount" );

        asset market_fee{0, eos_quant.symbol};
        if(market->buy_fee_rate > 0){//减去交易所手续费
            market_fee = calcfee(eos_quant, market->buy_fee_rate);
        }

        action(//给交易所账户转入EOS
                permission_level{ payer, N(active) },
                market->quote.contract, N(transfer),
                std::make_tuple(payer, market->exchange_account, quant_after_fee, std::string("send EOS to ET included fee:" + to_string(market_fee)))
        ).send();

        quant_after_fee -= market_fee;
        eosio_assert( quant_after_fee.amount > 0, "quant_after_fee2 must a positive amount " );

        print("\nquant_after_fee:");
        quant_after_fee.print();

        asset token_out{0,token_symbol};
        _market.modify( *market, 0, [&]( auto& es ) {
            token_out = es.convert( quant_after_fee,  token_symbol);
            es.quote.balance += market_fee;

            statsfee(es, market_fee, asset{0, token_symbol});
        });
        eosio_assert( token_out.amount > 0, "must reserve a positive amount" );

        action(//交易所账户转出代币
                permission_level{ market->exchange_account, N(active) },
                market->base.contract, N(transfer),
                std::make_tuple(market->exchange_account, payer, token_out, std::string("receive token from ET"))
        ).send();
    }




    /*  卖代币,收取万分之markets.sell_fee_rate手续费,由交易池所有股东共享
     *  receiver: 		    卖币账号,接收EOS
     *  token_contract: 	代币属于哪个合约,如TEST代币是issuemytoken部署创建的
     *  quant:			    想要卖出的quant个代币
     *  fee_account:		收取手续费的账号,receiver==fee_account相当于无手续费
     *  fee_rate:		    手续费率:[0,10000),如:50等同于万分之50; 0等同于无手续费
     * */
    void exchange::selltoken( account_name receiver, account_name token_contract, asset quant ,account_name fee_account,int64_t fee_rate){
        require_auth( receiver );

        eosio_assert(quant.symbol.is_valid(), "invalid token_symbol");
        eosio_assert(quant.symbol != S(4, EOS), "eos_quant symbol must not be EOS");
        eosio_assert(quant.amount > 0, "cannot sell negative byte" );
        eosio_assert(fee_rate >= 0 && fee_rate < max_fee_rate, "invalid fee_rate");

        markets _market(_self,_self);

        uint128_t idxkey = (uint128_t(token_contract) << 64) | quant.symbol.value;

        auto idx = _market.template get_index<N(idxkey)>();
        auto market = idx.find(idxkey);
        eosio_assert(market!=idx.end(),"token market does not exist");

        auto quant_after_fee = quant;

        if(fee_rate > 0 && receiver != fee_account ){
            auto fee = calcfee(quant, fee_rate);
            quant_after_fee -= fee;

            action(//向小费账户转入token手续费
                    permission_level{ receiver, N(active) },
                    market->base.contract, N(transfer),
                    std::make_tuple(receiver, fee_account, fee, std::string("send token fee to fee_account:"+to_string(fee)) )
            ).send();
        }

        eosio::asset market_fee{0, quant.symbol};
        if(market->sell_fee_rate > 0){//交易所手续费
            market_fee = calcfee(quant, market->sell_fee_rate);
        }

        action(//向交易所账户转入代币
                permission_level{ receiver, N(active) },
                market->base.contract, N(transfer),
                std::make_tuple(receiver, market->exchange_account, quant_after_fee, std::string("send EOS to ET included fee:"+to_string(market_fee)) )
        ).send();

        quant_after_fee -= market_fee;
        eosio_assert( quant_after_fee.amount > 0, "quant_after_fee must a positive amount" );

        print("\nquant_after_fee:");
        quant_after_fee.print();

        asset tokens_out{0,market->quote.balance.symbol};
        _market.modify( *market, 0, [&]( auto& es ) {
            tokens_out = es.convert( quant_after_fee, market->quote.balance.symbol);
            es.base.balance += market_fee;

            statsfee(es, asset{0, market->quote.balance.symbol},market_fee);
        });

        eosio_assert( tokens_out.amount > 0, "token amount received from selling EOS is too low" );

        action(//交易所账户转出EOS
                permission_level{ market->exchange_account, N(active) },
                market->quote.contract, N(transfer),
                std::make_tuple(market->exchange_account, receiver, tokens_out, std::string("receive EOS from ET") )
        ).send();

    }

    /*  参与做庄,往交易池注入EOS和token代币,享受手续费分红,最少注入交易池的1/100,最多做庄人数为markets.banker_max_number
     *  account:		    支付账号,从这个账号转出当前市场价格的代币和EOS到bancor池账号中
     *  quant:			    新增的EOS量
     *  token_contract: 	代币属于哪个合约,如TEST代币是issuemytoken部署创建的
     *  token_symbol:		新增的代币符号
     * */
    void exchange::addtoken( account_name account,asset quant, account_name token_contract,symbol_type token_symbol ) {
        require_auth( account );

        eosio_assert(quant.amount > 0, "must purchase a positive amount" );
        eosio_assert(quant.symbol == S(4, EOS), "quant symbol must be EOS");
        eosio_assert(token_symbol.is_valid(), "invalid token_symbol");

        markets _market(_self,_self);
        uint128_t idxkey = (uint128_t(token_contract) << 64) | token_symbol.value;

        auto idx = _market.template get_index<N(idxkey)>();
        auto market = idx.find(idxkey);
        eosio_assert(market!=idx.end(),"token market does not exist");

        eosio_assert(quant >= market->quote.balance/100, "eos_quant must be more than 1/100 of current market->quote.balance");//当前投入必须大于等于交易池的1%

        asset token_out = (market->base.balance * quant.amount) / market->quote.balance.amount;//market->base.balance, 是 asset已经防止溢出
        eosio_assert(token_out.amount > 0 , "token_out must reserve a positive amount");

        eosio_assert(market->support_banker, "not support_banker");

        shareholders _shareholders(_self, _self);
        auto idx_shareholders = _shareholders.template get_index<N(idxkey)>();
        auto shareholder = idx_shareholders.find(idxkey);
        eosio_assert(shareholder!=idx_shareholders.end(),"token market does not exist");
        std::map<account_name, holderinfo> map1 = shareholder->map_acc_info;

        if(map1.find(account) == map1.end()){
            eosio_assert(map1.size() < market->banker_max_number, "addtoken number over limit");//判断是否有参股空席
        }

//        //调试信息
//        market->quote.balance.print();
//        market->base.balance.print();
//        shareholder->total_quant.print();

        double quote=market->quote.balance.amount,base=market->base.balance.amount;
        for(auto itr = map1.begin(); itr != map1.end(); itr++){
//            itr->second.eos_holding.print();//调试信息

            //token的计算一定要放在eos之前,因为eos计算之后,eos_holding会改变
            itr->second.token_holding.amount = base * itr->second.eos_holding.amount / shareholder->total_quant.amount;
            eosio_assert(itr->second.token_holding.amount>0 && itr->second.token_holding.amount<=base, "division overflow");

            //eos持有量按占比重新计算
            itr->second.eos_holding.amount = quote * itr->second.eos_holding.amount / shareholder->total_quant.amount;
            eosio_assert(itr->second.eos_holding.amount>0 && itr->second.eos_holding.amount<=quote, "division overflow 2");
        }

        _shareholders.modify( *shareholder, account, [&]( auto& es ) {//保存信息
            es.total_quant = market->quote.balance + quant;//新增股东
            if(map1.find(account) == map1.end()){
                map1[account].eos_in = quant;
                map1[account].token_in = token_out;
                map1[account].eos_holding = quant;
                map1[account].token_holding = token_out;
            }else{
                map1[account].eos_in += quant;
                map1[account].token_in += token_out;
                map1[account].eos_holding += quant;
                map1[account].token_holding += token_out;
            }
            es.map_acc_info = map1;
        });


        action(//给交易所账户转入EOS
                permission_level{ account, N(active) },
                market->quote.contract, N(transfer),
                std::make_tuple(account, market->exchange_account, quant, std::string("add EOS to ET"))
        ).send();

        action(//给交易所账户转入token
                permission_level{ account, N(active) },
                market->base.contract, N(transfer),
                std::make_tuple(account, market->exchange_account, token_out, std::string("add token to ET"))
        ).send();

        _market.modify( *market, 0, [&]( auto& es ) {
            es.quote.balance += quant;//是 asset已经防止溢出
            es.base.balance += token_out;//是 asset已经防止溢出
        });

    }

    /*  庄家取走代币,必须全额取走,会根据比例获得交易手续费分红
     *  account:		    支付账号,向这个账号转入当前市场价格的代币和EOS
     *  quant:			    减少的EOS量
     *  token_contract: 	代币属于哪个合约,如TEST代币是issuemytoken部署创建的
     *  token_symbol:		减少的代币符号
     * */
    void exchange::subtoken( account_name account, asset quant, account_name token_contract,symbol_type token_symbol ) {
        require_auth( account );

        eosio_assert(quant.symbol == S(4, EOS), "quant symbol must be EOS");

        markets _market(_self,_self);
        uint128_t idxkey = (uint128_t(token_contract) << 64) | token_symbol.value;

        auto idx = _market.template get_index<N(idxkey)>();
        auto market = idx.find(idxkey);
        eosio_assert(market != idx.end(), "token market does not exist 1");

        shareholders _shareholders(_self, _self);
        auto idx_shareholders = _shareholders.template get_index<N(idxkey)>();
        auto shareholder = idx_shareholders.find(idxkey);
        eosio_assert(shareholder != idx_shareholders.end(), "token market does not exist 2");

        double quote = market->quote.balance.amount, base = market->base.balance.amount;
        std::map<account_name, holderinfo> map1 = shareholder->map_acc_info;

        eosio_assert(map1.find(account) != map1.end(), "account is not exist");//账号必须存在才可以取出

        if(map1.size() == 1){
            quant = market->quote.balance;
        }else{
            //调试信息
//            market->quote.balance.print();
//            market->base.balance.print();
//            shareholder->total_quant.print();

            for (auto itr = map1.begin(); itr != map1.end(); itr++) {
//                itr->second.eos_in.print();

                //token的计算一定要放在eos之前,因为eos计算之后,eos_holding会改变
                itr->second.token_holding.amount = base * itr->second.eos_holding.amount / shareholder->total_quant.amount;
                eosio_assert(itr->second.token_holding.amount>0 && itr->second.token_holding.amount<=base, "division overflow");//这里是否要判断大于0

                itr->second.eos_holding.amount = quote * itr->second.eos_holding.amount / shareholder->total_quant.amount;
                eosio_assert(itr->second.eos_holding.amount>0 && itr->second.eos_holding.amount<=quote, "division overflow 2");
            }

            quant = map1[account].eos_holding;//一次性取走
        }


        int64_t token_out = 0;
        //大于交易池的总量,表示取消该交易池
        if (quant >= market->quote.balance) {
            quant = market->quote.balance;
            token_out = market->base.balance.amount;
        } else {
            double base = market->base.balance.amount, quote = market->quote.balance.amount, quant_tmp = quant.amount;

            token_out = (base * quant_tmp) / quote;

            eosio_assert(token_out < market->base.balance.amount, "token_out should less than market->base.balance");
            eosio_assert(token_out > 0, "token_out must reserve a positive amount");

                //减少资金池后,要保证币价波动小于等于万分之一: |base/quote - (base-token_out)/(quote-quant)| / (base/quote) <= 1/10000
            eosio_assert(
                        std::fabs(base / quote - (base - token_out) / (quote - quant_tmp)) / (base / quote) <= 0.0001,
                        "ratio before and after should less than or equal to 1/10000");
        }

        action(//交易所账户转出EOS
                permission_level{ market->exchange_account, N(active) },
                market->quote.contract, N(transfer),
                std::make_tuple( market->exchange_account, account, quant, std::string("receive EOS from ET"))
        ).send();

        action(//交易所账户转出token
                permission_level{ market->exchange_account, N(active) },
                market->base.contract, N(transfer),
                std::make_tuple(market->exchange_account, account, asset{token_out, token_symbol}, std::string("receive token from ET"))
        ).send();

        if(token_out == market->base.balance.amount){//全部取走,相当于取消该币的交易市场
            _market.erase(*market);
            _shareholders.erase(*shareholder);
        }else{
            _market.modify( *market, 0, [&]( auto& es ) {
                es.quote.balance -= quant;
                es.base.balance.amount -= token_out;
            });

            _shareholders.modify( *shareholder, account, [&]( auto& es ) {//保存信息
                es.total_quant = market->quote.balance;
                map1.erase(account);//规定一次性取走,所以直接擦除掉
                es.map_acc_info = map1;
            });
        }

    }

    /* 设置参数
     *  token_contract: 	代币属于哪个合约,如TEST代币是issuemytoken部署创建的
     *  token_symbol:		减少的代币符号
     *  paramname:          设置参数的名称
     *  param:              设置的参数
     * */
    void exchange::setparam(account_name token_contract,symbol_type token_symbol, string paramname, string param){
        require_auth( _self );

        markets _market(_self,_self);
        uint128_t idxkey = (uint128_t(token_contract) << 64) | token_symbol.value;

        auto idx = _market.template get_index<N(idxkey)>();
        auto market = idx.find(idxkey);
        eosio_assert(market != idx.end(), "token market does not exist");

//        print("\nparamname",paramname);
//        print("\nparam",param);
        if(!strcmp(paramname.c_str(),"buy_fee_rate")){
            _market.modify( *market, 0, [&]( auto& es ) {
                es.buy_fee_rate = atol(param.c_str());
                eosio_assert(es.buy_fee_rate >= 0 && es.buy_fee_rate < max_fee_rate, "invalid buy_fee_rate, 0<= buy_fee_rate < 10000");
            });
        }else if(!strcmp(paramname.c_str(),"sell_fee_rate")){
            _market.modify( *market, 0, [&]( auto& es ) {
                es.sell_fee_rate = atol(param.c_str());
                eosio_assert(es.sell_fee_rate >= 0 && es.sell_fee_rate < max_fee_rate, "invalid sell_fee_rate, 0<= sell_fee_rate < 10000");
            });
        }else if(!strcmp(paramname.c_str(),"support_banker")){
            _market.modify( *market, 0, [&]( auto& es ) {
                if(!param.compare("false")){
                    es.support_banker = false;
                }else{
                    es.support_banker = true;
                }
            });
        }else if(!strcmp(paramname.c_str(),"banker_max_number")){
            _market.modify( *market, 0, [&]( auto& es ) {
                es.banker_max_number = atol(param.c_str());
            });
        }else{
            eosio_assert(false, "paramname not exists");
        }
    }

    /* 暂停交易所,用于交易所功能升级等情况
     * 1.调用pause暂停交易所,把markets的所有数据保存到markets1中,清空markets的数据;
     * 2.修改markets结构,重新发布合约,调用restart重启交易所
     * */
    void exchange::pause(){
//        require_auth( _self );
//
//        markets1 _markets1(_self, _self);
//        markets _markets(_self, _self);
//
//        for(auto itr=_markets.begin(); itr != _markets.end(); ){
//            _markets1.emplace( _self, [&]( auto& m ) {
//                m.id = itr->id;
//                m.idxkey = itr->idxkey;
//                m.supply = itr->supply;
//                m.base = itr->base;
//                m.quote = itr->quote;
//                m.exchange_account = itr->exchange_account;
//                m.buy_fee_rate = itr->buy_fee_rate;
//                m.sell_fee_rate = itr->sell_fee_rate;
//                m.date = itr->date;
//                m.total_fee = itr->total_fee;
//                m.today_fee = itr->today_fee;
//                m.yesterday_fee = itr->yesterday_fee;
//                m.support_banker = itr->support_banker;
//                m.banker_max_number = itr->banker_max_number;
//            });
//
//            _markets.erase(itr);
//            itr=_markets.begin();
//        }
    }
    /* 重启交易所,配合pause一起使用
     * 1.调用pause暂停交易所,把markets的所有数据保存到markets1中,清空markets的数据;
     * 2.修改markets结构,重新发布合约,调用restart重启交易所
     * */
    void exchange::restart(){
//        require_auth( _self );
//
//        markets1 _markets1(_self, _self);
//        markets _markets(_self, _self);
//
//        for(auto itr=_markets1.begin(); itr != _markets1.end(); ){
//            _markets.emplace( _self, [&]( auto& m ) {
//                m.id = itr->id;
//                m.idxkey = itr->idxkey;
//                m.supply = itr->supply;
//                m.base = itr->base;
//                m.quote = itr->quote;
//                m.exchange_account = itr->exchange_account;
//                m.buy_fee_rate = itr->buy_fee_rate;
//                m.sell_fee_rate = itr->sell_fee_rate;
//                m.date = itr->date;
//                m.total_fee = itr->total_fee;
//                m.today_fee = itr->today_fee;
//                m.yesterday_fee = itr->yesterday_fee;
//                m.support_banker = itr->support_banker;
//                m.banker_max_number = itr->banker_max_number;
//            });
//
//            _markets1.erase(itr);
//            itr=_markets1.begin();
//        }
    }


    asset exchange::calcfee(asset quant, uint64_t fee_rate){
        asset fee = quant * fee_rate / max_fee_rate; // 万分之fee_rate,fee 是 asset已经防止溢出
        if(fee.amount < 1){
            fee.amount = 1;//最少万分之一
        }

        return fee;
    }

    /** 统计手续费
     * */
    void exchange::statsfee(exchange_state &market, asset eos_fee, asset token_fee){
        market.total_fee.eos_fee += eos_fee;
        market.total_fee.token_fee += token_fee;

        uint32_t nowtime = now();
        uint32_t time = nowtime - market.date.utc_seconds;
        if(time >= 2*day){//大于两天,即中间隔了两天没有交易,则昨日收益为0
            market.date = time_point_sec(nowtime/day*day);
            market.today_fee.eos_fee = eos_fee;
            market.today_fee.token_fee = token_fee;
            market.yesterday_fee.eos_fee.amount = 0;
            market.yesterday_fee.token_fee.amount = 0;
        }else if(time >= day){//大于一天,说明已经是隔日
            market.date = time_point_sec(nowtime/day*day);
            market.yesterday_fee = market.today_fee;//重新赋值昨日收益

            market.today_fee.eos_fee = eos_fee;     //重新计算新一天的收益
            market.today_fee.token_fee = token_fee;
        }else{
            market.today_fee.eos_fee += eos_fee;    //还没有隔日,继续累加
            market.today_fee.token_fee += token_fee;
        }
    }

    string exchange::to_string(asset quant) {
        string retstr = "";
        int64_t p = (int64_t)quant.symbol.precision();
        int64_t p10 = 1;
        while( p > 0  ) {
            p10 *= 10; --p;
        }
        p = (int64_t)quant.symbol.precision();

        char fraction[p+1];
        fraction[p] = '\0';
        auto change = quant.amount % p10;

        for( int64_t i = p -1; i >= 0; --i ) {
            fraction[i] = (change % 10) + '0';
            change /= 10;
        }

        char s[256]={0};

        sprintf(s, "%llu.", quant.amount / p10 );
        printi( quant.amount / p10 );
        retstr += s;
        retstr += fraction;
        retstr +=" ";

        auto sym = quant.symbol.value;
        sym >>= 8;
        for( int i = 0; i < 7; ++i ) {
            char c = (char)(sym & 0xff);
            if( !c ) break;
            retstr += c;
            sym >>= 8;
        }

        return retstr;

    }

}


EOSIO_ABI( etb::exchange, (create)(buytoken)(selltoken)(addtoken)(subtoken)(setparam)(pause)(restart))
