# texas hold'em poker

## compile
依赖libevent库
ubuntu下:
```bash
sudo apt-get install libevent-dev
```

开发机环境下:
```bash
jumbo install libevent
```

## test
```bash
git clone http://gitlab.baidu.com/wangchuan02/texas_holdem.git
cd texas_holdem
make
./server &
telnet localhost 10000
```
## use websocket
```bash
cd proxy
npm install
node index.js
```
open (http://localhost:8899) with your browser


## unit test
```bash
make clean
make test
./test
```

## client usage
- r money raise
- c       call
- f       fold

## TODO
- table list
- check action

