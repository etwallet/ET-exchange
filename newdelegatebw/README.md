# newdelegatebw
账户权限管理体系:只授权某个权限给某个账户
    
方法一:新增某个权限,只授权给某个action,使用场景:如授权某个权限,该权限只能操作账户的抵押功能
1. 新增delegatebw权限,只能用于抵押功能      
cleos --wallet-url http://localhost:6666 --url http://localhost:8000 set account permission user11111111 delegatebw '{"threshold": 1, "keys":[{"key":"EOS5Q5n6N3MPat4jMDFmoCK6TgXCrj9QPzEKRT5bo7azAAGJt3i4y","weight":1}],"accounts":[],"waits":[]}' active

2. 查看新增权限:cleos --wallet-url http://localhost:6666 --url http://localhost:8000  get account user11111111            
permissions: 
     owner     1:    1 EOS6D1nt1aYtWPbfm5qburgRZc8gHVY6pwAcu812U9CnYigJN1saA
     active     1:    1 EOS7nnGJ7Ra911dwR1rQFw2MD2M8RkRPzUBtYb3qBmuYfaxbkUWmd 
     delegatebw     1:    1 EOS5Q5n6N3MPat4jMDFmoCK6TgXCrj9QPzEKRT5bo7azAAGJt3i4y

3. 给delegatebw绑定eosio的抵押功能(也可以绑定多个功能)       
cleos --wallet-url http://localhost:6666 --url http://localhost:8000 set action permission user11111111 eosio delegatebw delegatebw

4. 使用user11111111@delegatebw权限进行抵押,在未导入delegatebw私钥的情况下,下面的指令是失败的,导入私钥后才能成功,同时,使用delegatebw权限不能执行其他action       
cleos --wallet-url http://localhost:6666 --url http://localhost:8000 push action eosio delegatebw '["user11111111","user22222222","1.0000 EOS","1.0000 EOS"]' -p user11111111@delegatebw        

5. 解除delegatebw绑定的功能    
cleos --wallet-url http://localhost:6666 --url http://localhost:8000 set action permission user11111111 eosio delegatebw NULL

6. 删除delegatebw权限(如果有未解绑功能,删除会失败)   
cleos  --wallet-url http://localhost:6666 --url http://localhost:8000  set account permission user11111111 delegatebw NULL active

方法二: 针对抵押,有过户和出借两种功能,如果只想授权某个执行者有出借功能,可以通过合约实现  
1. 修改编译合约:  
在newdelegatebw.cpp中,把所有的require_auth( N(delegatebwer) );中的delegatebwer修改为自己指定的账户,也就是抵押功能的执行者.    
eosiocpp -o /home/root1/work/eos/contracts/newdelegatebw/newdelegatebw.wast /home/root1/work/eos/contracts/newdelegatebw/newdelegatebw.cpp      
eosiocpp -g /home/root1/work/eos/contracts/newdelegatebw/newdelegatebw.abi /home/root1/work/eos/contracts/newdelegatebw/newdelegatebw.cpp

2. 使用账户4jscyydlhgij(范例,可自行修改)部署合约:                            		
cleos --wallet-url http://127.0.0.1:8900 --url http://18.144.16.89:8001 set contract 4jscyydlhgij /home/root1/work/eos/contracts/newdelegatebw -p 4jscyydlhgij

3. 4jscyydlhgij需要授权给合约:              		
cleos --wallet-url http://127.0.0.1:8900 --url http://18.144.16.89:8001 set account permission 4jscyydlhgij active '{"threshold": 1,"keys": [{"key": "EOS8PyUqfoVbJbDavz9rfmWtJsEX4hsEUeETy3HnVXEJZZzRfS82z1","weight": 1}],"accounts": [{"permission":{"actor":"4jscyydlhgij","permission":"eosio.code"},"weight":1}]}' owner -p 4jscyydlhgij

4. delegatebwer执行抵押,sujianzhong1是接收抵押的用户:                  		
cleos --wallet-url http://127.0.0.1:8900 --url http://18.144.16.89:8001 push action 4jscyydlhgij delegatebw '["sujianzhong1", "0.0000 EOS","0.0001 EOS"]' -p delegatebwer

5. 撤销抵押     
cleos --wallet-url http://127.0.0.1:8900 --url http://18.144.16.89:8001 push action 4jscyydlhgij undelegatebw '["sujianzhong1", "0.0000 EOS","0.0001 EOS"]' -p delegatebwer     
上面的delegatebwer只能执行4jscyydlhgij的出借抵押功能.
