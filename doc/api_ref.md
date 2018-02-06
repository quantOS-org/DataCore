# 快速入门

## 字典定义

### 市场代码（market code）

| 市场名称 | 市场代码 |
| --- | --- |
| 深交所 | SZ |
| 上交所 | SH |
| 中金所 | CFE |
| 郑商所 | CZC |
| 大商所 | DCE |
| 上期所 | SHF |
| 上金所 | SGE |
| 中证指数 | CSI |
| 港交所 | HK |

### 证券类型定义（inst_type）

|证券类型|inst_type|
|---|---|
|股票|1|
|封闭式基金|2|
|LOF基金|3|
|ETF基金|4|
|分级基金|5|
|国债商品|6|
|商品|7|
|可转债|8|
|回购|10|
|国债|11|
|地方政府债|12|
|金融债|13|
|企业债|14|
|公司债|15|
|资产支持证券|16|
|可交换债|17|
|可分离转债存债|18|
|政府支持机构债|19|
|转股换股|20|
|指数|100|
|股指期货|101|
|国债期货|102|
|商品期货|103|
|股指ETF期权|201|
|股指期货期权|202|
|商品期货期权|203|

### 标的代码（symbol）

由标的原始代码加市场代码组合而成，中间以&#39;.&#39;隔开，如&#39;000001.SH&#39;，多标的输入时以逗号 (&#39;,&#39;) 隔开，如：&#39;000001.SH, cu1709.SHF&#39;

### Bar类型（freq）

| Bar类型 | 说明 | 
| --- | --- |
| 15S | 15秒线 |
| 30S | 30秒线 |
| 1M | 1分钟线 |
| 5M | 5分钟线 |
| 15M | 15分钟线 |
| 1d | 日线|
| 1w | 周线|
| 1m | 月线|



## DataApi准备

如果已安装JAQS，可以跳过此步骤，按jaqs给出的方式导入DataApi。

python环境及依赖包的安装，请参考[DataApi安装指南](https://github.com/quantOS-org/DataApi)。

首先导入API模块。
```python
from DataApi import DataApi 
```
然后创建DataApi对象，连接DataServer。
```python
api = DataApi("127.0.0.1:8910") # 连接本地DataServer，地址根据DataServer的启动配置填写。
api.login("demo", "666666") # 连接自己启动的DataServer时，帐号密码任意。需要连接互联网远程服务器时，请注册帐号。
```

## 函数定义

### 实时行情数据查询 quote

输入标的代码（支持多标的），输出为最新市场行情，以dataframe格式返回，可指定返回字段，以fields参数标识。

输入参数：

1. 标的代码，支持多标的查询。
2. 需要返回字段(fields)，多字段以&#39;,&#39;隔开,为""时返回所有字段。缺省为""。

| 字段 | 类型 | 说明 | 缺省 |
| --- | --- | --- | --- |
| symbol | string | 标的代码，支持多标的查询 | 不可缺省 |
| fields | string | 返回字段 | &quot;&quot; |


查询示例：

```python
df, msg = api.quote(
                symbol="000001.SH, cu1709.SHF", 
                fields="open,high,low,last,volume")
```

输出字段：

| 字段 | 类型 | 说明 |
| --- | --- | --- |
| symbol | string | 标的代码 |
| code | string | 交易所原始代码 |
| date | int | 自然日,YYYYMMDD格式，如20170823 |
| time | int | 时间，精确到毫秒，如14:21:05.330记为142105330 |
| trade\_date | int | YYYYMMDD格式，如20170823 |
| open | double | 开盘价 |
| high | double | 最高价 |
| low | double | 最低价 |
| last | double | 最新价 |
| close | double | 收盘价 |
| volume | double | 成交量（总） |
| turnover | double | 成交金额（总） |
| vwap | double | 当日平均成交均价，计算公式为成交金额除以成交量 |
| oi | double | 持仓总量 |
| settle | double | 今结算价 |
| iopv | double | IOPV净值估值 |
| limit_up | double | 涨停价 |
| limit_down | double | 跌停价 |
| preclose | double | 昨收盘价 |
| presettle | double | 昨结算价 |
| preoi | double | 昨持仓 |
| askprice1 | double | 申卖价1 |
| askprice2 | double | 申卖价2 |
| askprice3 | double | 申卖价3 |
| askprice4 | double | 申卖价4 |
| askprice5 | double | 申卖价5 |
| bidprice1 | double | 申买价1 |
| bidprice2 | double | 申买价2 |
| bidprice3 | double | 申买价3 |
| bidprice4 | double | 申买价4 |
| bidprice5 | double | 申买价5 |
| askvolume1 | double | 申卖量1 |
| askvolume2 | double | 申卖量2 |
| askvolume3 | double | 申卖量3 |
| askvolume4 | double | 申卖量4 |
| askvolume5 | double | 申卖量5 |
| bidvolume1 | double | 申买量1 |
| bidvolume2 | double | 申买量2 |
| bidvolume3 | double | 申买量3 |
| bidvolume4 | double | 申买量4 |
| bidvolume5 | double | 申买量5 | 

###  实时行情订阅 subscribe
使用subscribe()函数订阅实时市场行情。

输入参数：

1. 标的代码，支持多标的查询。
2. 回调函数(func)，格式为func(k, v)。k为数据类型，目前只支持实时行情("quote")； v为实时行情数据，dictionary格式，数据含义参考quote函数输出字段定义。
3. 需要返回字段(fields)，多字段以&#39;,&#39;隔开，为""时返回所有字段。缺省为""。

|字段 | 类型|说明 |缺省值|
| --- | --- | --- | ---|
|symbol | string |标的代码，支持多标的查询 |不可缺省|
|func | function | 回调函数 |None| 
|fields | string |返回字段，多字段以','隔开；为""时返回所有字段|""|

使用示例：
```python
def on_quote(k,v):
    print v['symbol'] // 标的代码
    print v['last'] // 最新成交价
    print v['time'] // 最新成交时间

subs_list,msg = api.subscribe("000001.SH, cu1709.SHF",func=on_quote,fields="symbol,last,time,volume")
```
### 日线查询 daily

日线查询，支持停牌补齐、复权选择等选项。

输入参数：

1. 标的代码，支持多标的查询，必要参数
2. 开始日期 (start\_date)，string或者int类型：若为string类型，格式&#39;YYYY-MM-DD&#39;，如&#39;2017-08-01&#39;；若为int类型，格式为YYYYMMDD，如20170801。必要参数。
3. 结束日期 (end\_date)，string或者int类型：若为string类型，格式&#39;YYYY-MM-DD&#39;，如&#39;2017-08-01&#39;；若为int类型，格式为YYYYMMDD，如20170801。必要参数。
4. Bar类型(freq)，支持日线(&#39;1d&#39;)，周线(&#39;1w&#39;)和月线(&#39;1m&#39;)。缺省为日线(&#39;1d&#39;)。
5. 复权类型(adjust\_mode)，string类型，None不复权，&#39;post&#39;为后复权，后复权从股票上市开始算起。缺省为None。
6. 返回字段 (fields)，多字段以 &#39;,&#39; 隔开，为""时返回所有字段。缺省为""。

| 字段 | 类型 | 说明 | 缺省值 |
| --- | --- | --- | --- |
| symbol | string | 标的代码 ，支持多标的查询 | 不可缺省 |
| start\_date | int或者string | 开始日期 | 不可缺省 |
| end\_date | int或者string | 结束日期  | 不可缺省 |
| freq | string | 日线类型 | &quot;1d&quot; |
| adjust\_mode | string | 复权类型 | None |
| fields | string | 返回字段| &quot;&quot; |


查询示例：
```python
df, msg = api.daily(
                symbol="600832.SH, 600030.SH", 
                start_date="2012-10-26",
                end_date="2012-11-30", 
                fields="", 
                adjust_mode="post")
```
返回字段：

| 字段 | 类型 | 说明 |
| --- | --- | --- |
| symbol | string | 标的代码 |
| code | string | 交易所原始代码 |
| trade\_date | int | YYYYMMDD格式，如20170823 |
| freq | string | 日线类型 | 
| open | double | 开盘价 |
| high | double | 最高价 |
| low | double | 最低价 |
| close | double | 收盘价 |
| volume | double | 成交量 |
| turnover | double | 成交金额 |
| vwap | double | 成交均价 |
| settle | double | 结算价 |
| oi | double | 持仓量 |
| trade\_status | string | 交易状态 |


### 分钟线查询 bar

查询各种类型的分钟线,支持日内及历史bar查询，以dataframe格式返回查询结果。

输入参数：

1. 标的代码，支持多标的查询，必要参数。
2. 开始时间 (start\_time)，精确到秒，string或者int类型：若为string类型，格式为&#39;HH:MM:SS&#39;，如&#39;09:32:35&#39;；若为int类型，格式为HHMMSS，如93235。缺省为为开盘时间。
3. 结束时间 (end\_time)，精确到秒，string或者int类型：若为string类型，格式为&#39;HH:MM:SS&#39;，如&#39;09:32:35&#39;；若为int类型，格式为HHMMSS，如9323。缺省为当前时间（日内）或者收盘时间（历史）。
4. 交易日 (trade\_date)，string或者int类型：若为string类型，格式&#39;YYYY-MM-DD&#39;，如&#39;2017-08-01&#39;；若为int类型，格式为YYYYMMDD，如20170801。缺省为当前交易日。
5. Bar类型(freq)，支持一分钟线(&#39;1M&#39;)，五分钟线(&#39;5M&#39;)和十五分钟线(&#39;15M&#39;)。缺省为一分钟线 (&#39;1M&#39;)。
6. 返回字段 (fields)，多字段以 &#39;,&#39; 隔开，为""时返回所有字段。缺省为""。

| 字段 | 类型 | 说明 | 缺省值 |
| --- | --- | --- | --- |
| symbol | string | 标的代码，支持多标的查询 | 不可缺省 |
| start\_time | int或string | 开始时间 | 开盘时间 |
| end\_time | int或string | 结束时间 | 收盘时间 |
| trade\_date | int或string | 交易日 | 当前交易日 |
| freq | string | 分钟线类型 | &quot;1M&quot; |
| fields | string | 返回字段 | &quot;&quot; |


查询示例：
```python
df,msg = api.bar(
            symbol="600030.SH", 
            trade_date=20170928, 
            freq="5M",
            start_time="00:00:00",
            end_time="16:00:00",
            fields="")
```
返回字段：

| 字段 | 类型 | 说明 |
| --- | --- | --- |
| symbol | string | 标的代码 |
| code | string | 交易所原始代码 |
| date | int | 自然日,YYYYMMDD格式，如20170823 |
| time | int | 时间，精确到毫秒，如14:21:05.330记为142105330 |
| trade\_date | int | YYYYMMDD格式，如20170823 |
| freq | string | bar类型 |
| open | double | bar内开盘价 |
| high | double | bar内最高价 |
| low | double | bar内最低价 |
| close | double | bar内收盘价 |
| volume | double | bar内成交量 |
| turnover | double | bar内成交金额 |
| vwap | double | bar内成交均价 |
| oi | double | 当前持仓量 |
| settle | double | 结算价 |


### bar quote查询 bar_quote

在分钟线基础上再加入该分钟结束前最后一笔的行情信息（主要是ask,bid信息），以dataframe格式返回查询结果。

输入参数：

1. 标的代码，支持多标的查询，必要参数。
2. 开始时间 (start\_time)，精确到秒，string或者int类型：若为string类型，格式为&#39;HH:MM:SS&#39;，如&#39;09:32:35&#39;；若为int类型，格式为HHMMSS，如93235。缺省为为开盘时间。
3. 结束时间 (end\_time)，精确到秒，string或者int类型：若为string类型，格式为&#39;HH:MM:SS&#39;，如&#39;09:32:35&#39;；若为int类型，格式为HHMMSS，如9323。缺省为当前时间（日内）或者收盘时间（历史）。
4. 交易日 (trade\_date)，string或者int类型：若为string类型，格式&#39;YYYY-MM-DD&#39;，如&#39;2017-08-01&#39;；若为int类型，格式为YYYYMMDD，如20170801。缺省为当前交易日。
5. Bar类型(freq)，支持一分钟线(&#39;1M&#39;)，五分钟线(&#39;5M&#39;)和十五分钟线(&#39;15M&#39;)。缺省为一分钟线 (&#39;1M&#39;)。
6. 返回字段 (fields)，多字段以 &#39;,&#39; 隔开，为""时返回所有字段。缺省为""。

| 字段 | 类型 | 说明 | 缺省值 |
| --- | --- | --- | --- |
| symbol | string | 标的代码 ，支持多标的查询 | 不可缺省 |
| start\_time | int或string | 开始时间 | 开盘时间 |
| end\_time | int或string | 结束时间 | 收盘时间 |
| trade\_date | int或string | 交易日 | 当前交易日 |
| freq | string | 分钟线类型 | &quot;1M&quot; |
| fields | string | 返回字段 | &quot;&quot; |


查询示例：

```python
df,msg = api.bar_quote(
                    symbol="000001.SH,cu1709.SHF",  
                    start_time = "09:56:00", 
                    end_time="13:56:00", 
                    trade_date=20170823, 
                    freq= "5M",
                    fields="open,high,low,last,volume")
```
返回字段：

| 字段 | 类型 | 说明 |
| --- | --- | --- |
| symbol | string | 标的代码 |
| code | string | 交易所原始代码 |
| date | int | 自然日，YYYYMMDD格式，如20170823 |
| time | int | 时间，精确到毫秒，如14:21:05.330记为142105330 |
| trade\_date | int | 交易日，YYYYMMDD格式，如20170823 |
| freq | string | bar类型 |
| open | double | bar内开盘价 |
| high | double | bar内最高价 |
| low | double | bar内最低价 |
| close | double | bar内收盘价 |
| volume | double | bar内成交量 |
| turnover | double | bar内成交金额 |
| vwap | double | bar内成交均价 |
| oi | double | 当前持仓量 |
| settle | double | 结算价 |
| askprice1 | double | 申卖价1 |
| askprice2 | double | 申卖价2 |
| askprice3 | double | 申卖价3 |
| askprice4 | double | 申卖价4 |
| askprice5 | double | 申卖价5 |
| bidprice1 | double | 申买价1 |
| bidprice2 | double | 申买价2 |
| bidprice3 | double | 申买价3 |
| bidprice4 | double | 申买价4 |
| bidprice5 | double | 申买价5 |
| askvolume1 | double | 申卖量1 |
| askvolume2 | double | 申卖量2 |
| askvolume3 | double | 申卖量3 |
| askvolume4 | double | 申卖量4 |
| askvolume5 | double | 申卖量5 |
| bidvolume1 | double | 申买量1 |
| bidvolume2 | double | 申买量2 |
| bidvolume3 | double | 申买量3 |
| bidvolume4 | double | 申买量4 |
| bidvolume5 | double | 申买量5 |

### 历史Tick查询 tick
查询历史tick数据，以dataframe格式返回查询结果。

**注意**，TusharePro在线服务不支持tick查询，仅本地部署DataServer(1.2版及以上)支持。

输入参数：

1. 标的代码，支持多标的查询，必要参数。
2. 开始时间 (start\_time)，精确到秒，string或者int类型：若为string类型，格式为&#39;HH:MM:SS&#39;，如&#39;09:32:35&#39;；若为int类型，格式为HHMMSS，如93235。缺省为为开盘时间。
3. 结束时间 (end\_time)，精确到秒，string或者int类型：若为string类型，格式为&#39;HH:MM:SS&#39;，如&#39;09:32:35&#39;；若为int类型，格式为HHMMSS，如9323。缺省为收盘时间（历史）。
4. 交易日 (trade\_date)，string或者int类型：若为string类型，格式&#39;YYYY-MM-DD&#39;，如&#39;2017-08-01&#39;；若为int类型，格式为YYYYMMDD，如20170801。缺省为0。
5. 返回字段 (fields)，多字段以 &#39;,&#39; 隔开，为""时返回所有字段。缺省为""。

| 字段 | 类型 | 说明 | 缺省值 |
| --- | --- | --- | --- |
| symbol | string | 标的代码，支持多标的查询 | 不可缺省 |
| start\_time | int或string | 开始时间 | 开盘时间 |
| end\_time | int或string | 结束时间 | 收盘时间 |
| trade\_date | int或string | 交易日 | 0 |
| fields | string | 返回字段 | &quot;&quot; |

查询示例：

```python
df,msg = api.tick(symbol='600030.SH,000002.SZ',trade_date = 20171221, 
                  start_time = '9:29:00',end_time = '09:32:15',
                  fields='volume,date,trade_date,last')
```

输出字段：

| 字段 | 类型 | 说明 |
| --- | --- | --- |
| symbol | string | 标的代码 |
| code | string | 交易所原始代码 |
| date | int | 自然日,YYYYMMDD格式，如20170823 |
| time | int | 时间，精确到毫秒，如14:21:05.330记为142105330 |
| trade\_date | int | YYYYMMDD格式，如20170823 |
| open | double | 开盘价 |
| high | double | 最高价 |
| low | double | 最低价 |
| last | double | 最新价 |
| close | double | 收盘价 |
| volume | double | 成交量（总） |
| turnover | double | 成交金额（总） |
| vwap | double | 当日平均成交均价，计算公式为成交金额除以成交量 |
| oi | double | 持仓总量 |
| settle | double | 今结算价 |
| iopv | double | IOPV净值估值 |
| limit_up | double | 涨停价 |
| limit_down | double | 跌停价 |
| preclose | double | 昨收盘价 |
| presettle | double | 昨结算价 |
| preoi | double | 昨持仓 |
| askprice1 | double | 申卖价1 |
| askprice2 | double | 申卖价2 |
| askprice3 | double | 申卖价3 |
| askprice4 | double | 申卖价4 |
| askprice5 | double | 申卖价5 |
| bidprice1 | double | 申买价1 |
| bidprice2 | double | 申买价2 |
| bidprice3 | double | 申买价3 |
| bidprice4 | double | 申买价4 |
| bidprice5 | double | 申买价5 |
| askvolume1 | double | 申卖量1 |
| askvolume2 | double | 申卖量2 |
| askvolume3 | double | 申卖量3 |
| askvolume4 | double | 申卖量4 |
| askvolume5 | double | 申卖量5 |
| bidvolume1 | double | 申买量1 |
| bidvolume2 | double | 申买量2 |
| bidvolume3 | double | 申买量3 |
| bidvolume4 | double | 申买量4 |
| bidvolume5 | double | 申买量5 | 


### 参考数据查询 query

参考数据查询，输入查询类型及查询参数，返回结果为dataframe格式。具体查询方法参见[tushare网站](http://tushare.org/pro)。

输入参数：

1.  查询类型 (view)
2.  查询参数 (filter)，视查询类型而定
3.  返回字段 (fields)，视查询类型而定

| 字段 | 类型 | 说明 | 缺省值 |
| --- | --- | --- | --- |
| view | string | 参考数据查询类型，详见参考数据文档 | 不可缺省 | 
| filter | string | 查询条件，多条件以"&"隔开，详见参考数据文档 | 不可缺省 |
| fields | string | 返回字段，多字段以','隔开，若传入为""，则返回view指定的must返回字段，详见参考数据文档| "" |
