# ET-exchange
基于EOS的去中心化交易平台<br>

[ET开放式去中心化交易平台白皮书](https://github.com/eostoken/ET-exchange/blob/master/WhitePaper.md)

telegram：https://t.me/etex_official <br>
微信：tao709308469


#etbexchange合约说明
1. 创建ETB代币:create() 

2. 发币:issue( account_name from,account_name to,asset quantity)     
from: EOS转出账户   
to: ETB接收账户     
quantity: EOS转出数量   
功能描述:   
a 从from转出quantity个EOS到合约账户,quantity必须为整数如1.0000 EOS   
b 给接收者发币(1.30-1.00)*quantity个ETB    
c 合伙人奖励:from!=to时from有奖励机制,统计from给他人的EOS转出量,如超过5000个EOS,则(1):如之前已超过5000,则奖励:当笔EOS转出量*(1.30~1.00)*5%个ETB(2)如之前未超5000,则奖励:EOS转出总量*(1.30~1.00)*5%个ETB  
d 团队奖励: 当笔EOS转出量*(1.30-1.00)*10%个ETB    
e 合伙人奖励和团队奖励冻结在合约账户里面,当活动结束后每天解冻1/365,通过claimrewards索取奖励.

3. 领取奖励:claimrewards( const account_name& owner )   
owner:领取奖励的账户

4. 转账:transfer( account_name from,account_name to,asset quantity,string memo )  
from:转出账户   
to:接收账户 
quantity:金额数量   
memo:备注信息


例子:     
第一天:from帮to购买1000个EOS,则to账户有1300=1000*1.30个ETB,合约账户有130个ETB,from无ETB;     
第二天:form帮自己购买4000个EOS,则from有5160=4000*1.29个ETB,合约账户余额646个ETB;     
第三天:from帮to购买4000个EOS,则to账户有6420=4000*1.28+1300个ETB,from账户有320=5000*1.28*0.05个ETB奖励(需要活动结束领取,可以通过下面的指令get table查看),合约账户有1158个ETB

     
指令:     
部署合约:cleos set contract etbexchanger /contracts/etbexchange -p etbexchanger          
创建代币:cleos push action etbexchanger create '[]' -p etbexchanger             
发币:cleos push action etbexchanger issue '["user11111111", "user22222222", "5000.0000 EOS"]' -p user11111111     
查看user11111111的奖励:cleos get table etbexchanger etbexchanger exchanges -L user11111111 -l 1      
查看所有账户奖励:cleos get table etbexchanger etbexchanger exchanges        
索取奖励:cleos push action etbexchanger claimrewards '["user11111111"]' -p user11111111             
