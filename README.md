# texas hold'em poker

# compile
依赖libevent berkeley-db
ubuntu下:
```bash
sudo apt-get install libevent-dev libdb-dev
git clone https://github.com/wangchuan3533/texas_holdem.git
cd texas_holdem
make
```

开发机环境下:
```bash
jumbo install libevent
git clone https://github.com/wangchuan3533/texas_holdem.git
cd texas_holdem
make
```

# test
## 如何运行
```bash
## server
./server &

## client 1
telnet localhost 10000
> ret x pass
> login x pass
> mk t

## client 2
telnet localhost 10000
> reg y pass
> login y pass
> join t

## client 3
telnet localhost 10000
> reg z pass
> login z pass
> join t
> start
```
## 客户端命令
### 房间相关
- reg user password   注册
- login user password 登录
- logout              登出
- mk table            新建房间
- join table          加入房间
- quit                退出当前房间
- exit                退出游戏
- show tables         查看房间
- show players        查看玩家
- show players in t   查看t房间中的玩家
- pwd                 查看当前房间
- help                查看帮助信息
- chat 'text'         房间内群聊(要用引号括起来)
- prompt 'text'       设置提示符

### 游戏操作
- start          开始游戏
- bet n          赌注(+ n, n可以是数学表达式，内置计算器支持)
- raise n        加注
- call           跟注
- fold           弃牌
- check          让牌
- all in         all in

### 计算器
- 支持加减乘除的基本运算，数字支持10进制和16进制(0x前缀)

## 使用 websocket代理

```bash
cd proxy
npm install
node index.js
```
用浏览器打开 http://localhost:8899/

## Online Demo
telnet 121.42.166.108 10000
or
http://121.42.166.108:8899/

## TODO
- burn card
- table start bug fix
- sendmsg() bug
- timeout bug
- bin log
- exit bug
- all in bug
