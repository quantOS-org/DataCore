# 安装指南

## 数据API安装

### 安装Python环境

如果本地还没有安装Python环境，强烈建议安装 [Anaconda](http://www.continuum.io/downloads "Anaconda")。

打开上面的网址，选择相应的操作系统，确定要按照的Python版本，一般建议用Python 2.7。

下载完成以后，按照图形界面步骤完成安装。在默认情况下，Anaconda会自动设置PATH环境。

### 安装依赖包

如果Python环境不是类似Anaconda的集成开发环境，我们需要单独安装依赖包，在已经有pandas/numpy包前提下，还需要有以下几个包：

- pyzmq
- msgpack_python
- python-snappy

可以通过单个安装完成，例如： pip install pyzmq

需要注意的是，python-snappy和msgpack-python这两个包在Windows上的安装需要比较多的编译依赖,建议从[这个网页](http://www.lfd.uci.edu/~gohlke/pythonlibs)下载编译好的包，然后安装:
```bash
pip install msgpack_python-0.4.8-cp27-cp27m-win_amd64.whl 
pip install python_snappy-0.5.1-cp27-cp27m-win_amd64.whl
```

## 服务端安装

DataServer由Scala开发，需要安装JDK8才能运行，建议从[该网页](http://www.oracle.com/technetwork/java/javase/downloads/jdk8-downloads-2133151.html)下载安装。

### 1. 准备行情源
目前支持通过CTP接入期货行情，通过万得宏汇接入股票行情。您需要准备相应的账户。

### 2. 解压缩mdlink安装包

linux安装包名格式为mdlink-[版本号]-linux.7z，如mdlink-1.0-linux.7z；
windows安装包名格式为mdlink-[版本号]-win32.zip，如mdlink-1.0-win32.zip。

程序目录及其含义如下：

#### bin目录
存放运行程序，包含以下几个程序：

+ mdlink_ctp：接收期货行情。
+ mdlink_tdf：接收万得宏汇行情。
+ mdlink2：用于行情汇总。
+ qms2：生成分钟线。

#### etc目录
存放配置及基础数据文件

+ jzs.json：程序运行配置文件。
+ calendar.csv：交易日历。
+ instrument.csv：标的信息。
+ market.csv：市场信息。

#### scripts目录
启动及关闭脚本，以linux为例，包含以下几个脚本：

+ start.sh:：启动脚本，依次启动qms2,mdlink2,mdlink_ctp,mdlink_tdf。
+ stop.sh：关闭脚本。
+ set-env.sh：为运行环境配置脚本。
+ jztsctrl：单程序启动关闭脚本。

#### lib目录
存放程序运行的依赖库。

#### data目录

+ log目录：存放运行日志文件。
+ tk目录：存放行情tick数据。
+ flow目录：存放CTP订阅信息文件。
+ ct目录：存放TDF的codetable文件。
+ tmp目录：存放进程pid文件。

### 3. mdlink/qms配置文件准备
程序配置存放在etc/jzs.json里面，根据自己的需求更改程序配置。
配置示例及说明如下：
```
{
    "global": {
        "mdlink_addr": "tcp://127.0.0.1:8700", // mdlink连接地址
        "qms_addr": "tcp://127.0.0.1:9000" //qms连接地址
    },

    "qms": {
        "id": "qms01", //qms名称
        "check_code": true, //qms重启时，是否对tick文件进行代码检查
        "save_tk": true, //是否保存tick文件，保存tick后，qms在重启时会根据保存的tick还原分钟线，否则重启后会丢失之前的分钟线数据
        "addr": "tcp://0.0.0.0:9000" // qms地址及端口
    },

    "mdlink": 
    {
        "mdlink2": { // 配置名称
            "pub_addr": "tcp://0.0.0.0:8700", //行情推送地址及端口
            "do_merge": true, // 是否进行行情合并
            "route": "MERGE", // mdlink类型，MERGE表示为行情合并mdlink
            "time_tolerance": 10000, 
            "sources": [ // 上游mdlink行情源及相应地址和端口
                { "addr": "tcp://127.0.0.1:10001", "id": "future1" },
                { "addr": "tcp://127.0.0.1:10002", "id": "stock1" }
            ]
        },

        "future1": {
            "pub_addr": "tcp://0.0.0.0:10001",
            "route": "CTP", // mdlink类型，CTP表示为mdlink_ctp读取的配置
            "source_id": 1, // source编号
            "sources": [ // CTP行情源配置，该配置由期货公司提供
                {
                    "id": "future1",
                    "route": "CTP",
                    "front": [
                        "180.168.146.187:10010",
                        "180.168.146.187:10011"
                    ],
                    "broker": "9999",
                    "investor": "xxxxx",
                    "passwd": "xxxxx",
                    "udp": false,
                    "multicast": false
                }
            ]
        },

        "stock1": {
            "pub_addr": "tcp://0.0.0.0:10002",
            "route": "TDF", // mdlink类型，"TDF"表示为mdlink_tdf读取的配置
            "source_id": 2,
            "sources": [ // TDF行情源配置，该配置由万得宏汇公司提供
                {
                    "id": "stock1",
                    "route": "TDF",
                    "addr": "xxx.xxx.xxx.xxx",
                    "port": xxxx,
                    "username": "TDXXXXXX",
                    "passwd": "xxxxxx",
                    "markets": "SZ-2;SH-2;CF-2"
                }
            ]
        }
    }
}
```

注意：jzs.json文件为json格式文件，上述示例中的注释仅展示用，实际使用json文件时需将注释内容删除。

### 4. 启动mdlink/qms
#### Linux下启动
cd 到 mdlink目录下，
运行：
```bash
$ ./scripts/start.sh
```
可以依次启动全部mdlink程序和qms程序。

如果需要单独启动某个程序，需要先运行： 
```bash
$ . scripts/set-env.sh
```
注意"."不可缺少。

然后运行"./bin/程序名 配置名"或者运行"jztsctrl 程序名 配置名 start"即可启动程序。
使用示例：
```bash
$ ./bin/qms2 qms2 > qms2.log&
$ ./bin/mdlink2 mdlink2 > mdlink2.log&
$ ./bin/mdlink_ctp future1 > future1.log&
$ ./bin/mdlink_tdf stock1  > stock1.log&
```
或者：
```bash
$ jztsctrl qms2 qms2 start
$ jztsctrl mdlink2 mdlink2 start
$ jztsctrl mdlink_ctp future1 start
$ jztsctrl mdlink_tdf stock1 start
```
#### Windows下启动
进入scripts目录下，双击运行start.bat。

如果需要单独启动某个程序，命令行模式下进入mdlink目录，
运行 "bin\程序名 配置名"

#### 检查log文件
程序正常启动后会在"data/log"目录下生成相应log文件，文件名格式为"配置名.YYMMDD-HHMMSS.PID"，
例如："future1.20171110-160015.19116" 表示由mdlink_ctp在2017年11月10日16点00分15秒启动future1源时生成的log，程序PID为19116。


### 5. 解压缩DataServer安装包。
安装包名格式为dataserver-[版本号].7z，如dataserver-1.0.7z

程序目录及其含义如下：

#### bin目录
存放dataserver启动程序。

#### etc目录
存放配置及基础数据文件。

+ dataserver-dev.conf：dataserver运行配置文件。
+ calendar.csv：交易日历。
+ instrument.csv：标的信息。
+ market.csv：市场信息。

#### lib目录
存放程序依赖库。

#### log
存放运行log文件。

#### script
存放运行及关闭脚本，以Linux为例

+ start.sh：启动脚本
+ stop.sh：关闭脚本

### 6. DataServer配置准备
程序配置存放在 dataserver-dev.conf 里面，根据自己的需求更改程序配置。

```
{
  "mdlink": { // 需要连接的mdlink地址
    "addr": "tcp://127.0.0.1:8700",
    "pub_addr": "tcp://127.0.0.1:8701"
  },

  "qms": { //需要连接的qms地址
    "addr": "tcp://127.0.0.1:9000"
  },

  "frontend" : { // dataserver地址（DataApi用）
    "zmqrpc_addr" : "tcp://0.0.0.0:8910"
  },

  "http_server" : { // websocket访问端口
    "port"      : "8912",
    "doc_root"  : "web/new"
  }
}
```
注意：dataserver-dev.conf文件为json格式文件，上述示例中的注释仅展示用，实际使用json文件时需将注释内容删除。

### 7. 启动DataServer
启动DataServer前需要确保mdlink和qms已经正常运行。

#### Linux下启动
cd 到 dataserver目录下，运行：
```bash
$ ./script/start.sh
```

#### Window下启动
在script下双击start.bat
