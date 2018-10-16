# ET-exchange
基于EOS的去中心化交易平台<br>

[ET开放式去中心化交易平台白皮书](https://github.com/eostoken/ET-exchange/blob/master/WhitePaper.md)

telegram：https://t.me/etex_official <br>
微信：tao709308469 <br><br>
![etlogo](https://github.com/eostoken/ET-exchange/blob/master/picture/lo1.png)
##
一. 合约接口说明:
1. 创建代币bancor池:     
void create(account_name payer, account_name exchange_account, asset eos_supply, account_name  token_contract,  asset token_supply);    
payer:			支付账号,从这个账号转出代币和EOS到bancor池账号    
exchange_account:	bancor池账号,TEST代币和EOS代币都在此账号名下:如:etbexchange1     
eos_supply:		初始化EOS数量:如: 100000.0000 EOS     
token_contract: 	代币属于哪个合约,如:issuemytoken部署创建了TEST代币       
token_supply:		初始化代币数量:如:1000000.0000 TEST代币    

2. 购买代币,收取万分之markets.buy_fee_rate手续费,由交易池所有股东共享  
void buytoken( account_name payer, asset eos_quant,account_name token_contract, symbol_type token_symbol, account_name fee_account,int64_t fee_rate);   
payer: 	买币账号    
eos_quant:		用quant个EOS购买代币  
token_contract: 	代币属于哪个合约,如TEST代币是issuemytoken部署创建的   
token_symbol:		想要购买的代币符号:如TEST  
fee_account:		收取手续费的账号    
fee_rate:		手续费率:[0,10000),如:50等同于万分之50; 0等同于无手续费   

3. 卖代币,收取万分之markets.sell_fee_rate手续费,由交易池所有股东共享  
void selltoken( account_name receiver, account_name token_contract, asset quant ,account_name fee_account,int64_t fee_rate);    
receiver: 		卖币账号,接收EOS  
token_contract: 	代币属于哪个合约,如TEST代币是issuemytoken部署创建的   
quant:			想要卖出的quant个代币   
fee_account:		收取手续费的账号    
fee_rate:		手续费率:[0,10000),如:50等同于万分之50; 0等同于无手续费   

4. 参与做庄,往交易池注入EOS和token代币,享受手续费分红,最少注入交易池的1/100,最多做庄人数为markets.banker_max_number    
void exchange::addtoken( account_name account,asset quant, account_name token_contract,symbol_type token_symbol )   
account:		支付账号,从这个账号转出当前市场价格的代币和EOS到bancor池账号中    
quant:			新增的EOS量         
token_contract: 	代币属于哪个合约,如TEST代币是issuemytoken部署创建的         
token_symbol:		新增的代币符号 
例如:当前市场1EOS可以买到10个TEST,那么增加1000个EOS时,会从account转出1000个EOS和10000个TEST到bancor池中  

5. 庄家取走代币,必须全额取走,会根据比例获得交易手续费分红    
void exchange::subtoken( account_name account, asset quant, account_name token_contract,symbol_type token_symbol )       
account:		支付账号,向这个账号转入当前市场价格的代币和EOS   
quant:			减少的EOS量     
token_contract: 	代币属于哪个合约,如TEST代币是issuemytoken部署创建的       
token_symbol:		减少的代币符号     
例如:当前市场1EOS可以买到10个TEST,那么减少1000个EOS时,会从bancor池中转出1000个EOS和10000个TEST到account中

6. 设置参数     
void exchange::setparam(account_name token_contract,symbol_type token_symbol, string paramname, string param);
token_contract: 	代币属于哪个合约,如TEST代币是issuemytoken部署创建的      
token_symbol:		减少的代币符号     
paramname:          设置参数的名称,如exchange_type      
param:              设置的参数           

7. 暂停交易所        
void exchange::pause();

8. 重启交易所        
void exchange::restart();

##
二. 交易所操作步骤:合约账号:etbexchanger,用于创建交易所;(可在主网上查看)

1.编译合约      
eosiocpp -o /eos/contracts/etbexchange/etbexchange.wast  /eos/contracts/etbexchange/etbexchange.cpp /eos/contracts/etbexchange/exchange_state.cpp       
eosiocpp -g /eos/contracts/etbexchange/etbexchange.api  /eos/contracts/etbexchange/etbexchange.cpp

2. 部署交易所合约  
eg: cleos  set contract etbexchanger /eos/contracts/etbexchange -p etbexchanger

3. 创建TEST代币bancor池:  
  
先授权给合约:     
cleos set account permission etbexchanger active '{"threshold": 1,"keys": [{"key": "EOS6mBLtJQv5Adv36dDkvWtPP7bqUNArwWXiZCT8711CUBPBTbdnR","weight": 1}],"accounts": [{"permission":{"actor":"etbexchanger","permission":"eosio.code"},"weight":1}]}' owner -p etbexchanger

从etbexchanger中转出4个EOS和100000000个TEST币到bancor池etbexchange1中:   
cleos  push action etbexchanger create '["etbexchanger","etbexchange1", "4.0000 EOS","issuemytoken","100000000.0000 TEST"]' -p etbexchanger

撤销授权:       
cleos set account permission etbexchanger active '{"threshold": 1,"keys": [{"key": "EOS6mBLtJQv5Adv36dDkvWtPP7bqUNArwWXiZCT8711CUBPBTbdnR","weight": 1}],"accounts": []}' owner -p etbexchanger

4. 查看交易所的币交易情况:     
cleos get table etbexchanger etbexchanger markets       
{   
  "rows": [{    
      "id": 0,  
      "idxkey": "0x04544553540000003015a4d94ba53176",   
      "supply": "100000000000000 ETBCORE",  
      "base": {     
        "contract": "issuemytoken",             //TEST币的合约账号,可查看该币的发币情况:get currency stats issuemytoken TEST        
        "balance": "104265457.2018 TEST",       //目前该资金池有 104265457.2018 TEST       
        "weight": "0.50000000000000000"     
      },        
      "quote": {        
        "contract": "eosio.token",              //EOS币的合约账号,可查看该币的发币情况:get currency stats eosio.token EOS       
        "balance": "3.8364 EOS",                //目前该资金池有 3.8364 EOS        
        "weight": "0.50000000000000000"     
      },        
      "exchange_account": "etbexchange1"        //资金池账号:etbexchange1,上面的TEST和EOS都在这个账号下     
    }],        
  "more": false     
}

cleos get table etbexchanger etbexchanger shareholders
{
  "rows": [{
      "id": 0,
      "idxkey": "0x044554420000000010149ba6a1ae4e56",
      "total_quant": "3.8364 EOS",
      "map_acc_info": [{
          "account": "user11111111",
          "info": {
            "eos_in": "3.8364 EOS",
            "token_in": "104265457.2018 TEST",
            "eos_holding": "3.8364 EOS",
            "token_holding": "104265457.2018 TEST"
          }
        }
      ]
    }],
    "more": false
}
##
三. 用户交易步骤:  
用户1:user11111111, 用户2:user22222222, 

1. 买币(买币前授权eosio.code给合约账号etbexchange1,买币后撤销权限)              

授权: cleos  set account permission user11111111 active '{"threshold": 1,"keys": [{"key": "EOS5EwrHc3V4aFjL2ADV9X246yoZgyFdpKj4spKxq3GJBhETndJum","weight": 1}],"accounts": [{"permission":{"actor":"etbexchanger","permission":"eosio.code"},"weight":1}]}' owner -p user11111111

买TEST币: cleos push action etbexchanger buytoken '["user11111111", "0.1000 EOS", "issuemytoken","4,TEST", "user11111111", "0"]' -p user11111111

撤销授权: cleos  set account permission user11111111 active '{"threshold": 1,"keys": [{"key": "EOS5EwrHc3V4aFjL2ADV9X246yoZgyFdpKj4spKxq3GJBhETndJum","weight": 1}],"accounts": []}' owner -p user11111111

2. 卖币   

授权: cleos  set account permission user11111111 active '{"threshold": 1,"keys": [{"key": "EOS5EwrHc3V4aFjL2ADV9X246yoZgyFdpKj4spKxq3GJBhETndJum","weight": 1}],"accounts": [{"permission":{"actor":"etbexchanger","permission":"eosio.code"},"weight":1}]}' owner -p user11111111

卖TEST币: cleos push action etbexchanger selltoken '["user11111111", "issuemytoken","12439024.3889 TEST", "user11111111", "0" ]' -p user11111111

撤销授权: cleos  set account permission user11111111 active '{"threshold": 1,"keys": [{"key": "EOS5EwrHc3V4aFjL2ADV9X246yoZgyFdpKj4spKxq3GJBhETndJum","weight": 1}],"accounts": []}' owner -p user11111111

3. 参与做庄,必须大于等于现有交易池的1/100         

授权: cleos  set account permission user11111111 active '{"threshold": 1,"keys": [{"key": "EOS5EwrHc3V4aFjL2ADV9X246yoZgyFdpKj4spKxq3GJBhETndJum","weight": 1}],"accounts": [{"permission":{"actor":"etbexchanger","permission":"eosio.code"},"weight":1}]}' owner -p user11111111

做庄: cleos push action etbexchanger addtoken '["user11111111", "1000.0000 EOS","issuemytoken", "4,TEST"]' -p user11111111

撤销授权: cleos  set account permission user11111111 active '{"threshold": 1,"keys": [{"key": "EOS5EwrHc3V4aFjL2ADV9X246yoZgyFdpKj4spKxq3GJBhETndJum","weight": 1}],"accounts": []}' owner -p user11111111

4. 撤回(必须一次性撤回)       

授权: cleos  set account permission user11111111 active '{"threshold": 1,"keys": [{"key": "EOS5EwrHc3V4aFjL2ADV9X246yoZgyFdpKj4spKxq3GJBhETndJum","weight": 1}],"accounts": [{"permission":{"actor":"etbexchanger","permission":"eosio.code"},"weight":1}]}' owner -p user11111111

卖TEST币: cleos push action etbexchanger subtoken '["user11111111", "0.0000 EOS","issuemytoken", "4,TEST" ]' -p user11111111

撤销授权: cleos  set account permission user11111111 active '{"threshold": 1,"keys": [{"key": "EOS5EwrHc3V4aFjL2ADV9X246yoZgyFdpKj4spKxq3GJBhETndJum","weight": 1}],"accounts": []}' owner -p user11111111


