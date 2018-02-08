# 基础数据

## 调用说明
- 通过api.query函数调用，第一个参数view需填入对应的接口名，如：`view="jz.instrumentInfo"` 
- 输入参数指的是filter参数里面的内容，通过'&'符号拼接，如：`filter="inst_type=&status=1&symbol="` 
- 输出参数指的是fields里面的内容，通过','隔开

样例代码：获取上市股票列表
```python
df, msg = api.query(
                view="jz.instrumentInfo", 
                fields="status,list_date, fullname_en, market", 
                filter="inst_type=1&status=1&symbol=", 
                data_format='pandas')
```

## 目前支持的接口及其含义

| 接口               | view                  | 分类       |
| ------------------ | --------------------- | ---------- |
| 证券基础信息表     | jz.instrumentInfo     | 基础信息   |
| 交易日历表         | jz.secTradeCal        | 基础信息   |
| 分配除权信息表     | lb.secDividend        | 股票       |
| 复权因子表         | lb.secAdjFactor       | 股票       |
| 停复牌信息表       | lb.secSusp            | 股票       |
| 行业分类表         | lb.secIndustry        | 股票       |
| 行业代码表         | lb.industryType       | 股票       |
| 指数基本信息表     | lb.indexInfo          | 指数       |
| 指数成份股表       | lb.indexCons          | 指数       |
| 公募基金净值表     | lb.mfNav              | 基金       |

## 接口查询（help.apiList）
 
### 接口说明
 
查询quantos支持那些业务接口。
 
### 输入参数
 
| 字段 | 字段中文名 | 类型 | 输入标志 | 说明 |
| --- | --- | --- | --- | --- |
| api | 参考数据接口 | String | N |  |
| name | 参考数据中文名 | String | N |  |
 
### 输入参数
 
| 字段 | 字段中文名 | 类型 | 输出标志 | 说明 |
| --- | --- | --- | --- | --- |
| api | 参考数据接口 | String | Y |  |
| name | 参考数据中文名 | String | Y |  |
| comment | 注释 | String | Y |  |

### 接口说明备注(适用于所有接口)

+ Y : 必须输入或输出
+ N : 可选输入或输出，如果是输出参数，可以在fields里面指定。

## 接口参数查询（help.apiParam）
 
查询具体某个接口的输入和输出参数。
 
### 输入参数
 
| 字段 | 字段中文名 | 类型 | 输入标志 | 说明 |
| --- | --- | --- | --- | --- |
| api | 参考数据接口 | String | N |  |
| ptype | 参数类型 | String | N | IN为输入参数，OUT为输出参数 |
| param | 参数代码 | String | N |  |
| must | 是否必要 | String | N | Y为必要参数，N为非必要参数 |
| pname | 参数中文名 | String | N |  |
 
### 输入参数
 
| 字段 | 字段中文名 | 类型 | 输出标志 | 说明 |
| --- | --- | --- | --- | --- |
| api | 参考数据接口 | String | Y |  |
| param | 参数代码 | String | Y |  |
| ptype | 参数类型 | String | Y | IN为输入参数，OUT为输出参数 |
| dtype | 数据类型 | String | Y | String为字符串，Int为整型，Double为浮点型 |
| must | 是否必要 | String | Y | Y为必要参数，N为非必要参数 |
| pname | 参数中文名 | String | Y |  |
| comment | 注释 | String | Y |  |

 
## 证券基础信息（jz.instrumentInfo）
 
证券基本信息
 
### 输入参数
 
| 字段 | 字段中文名 | 类型 | 输入标志 | 说明 |
| --- | --- | --- | --- | --- |
| inst_type | 证券类型  | Int | N |  |
| status | 上市状态 | Int | N |  |
| symbol | 证券代码 | String | N |  |
| market | 交易所 | String | N |  |
| start_delistdate | 退市阶段(开始日期) | String | N |  |
| end_delistdate | 退市阶段(结束日期) | String | N |  |
| trade_date | 交易日 | String | N |  |
 
### 输出参数
 
| 字段 | 字段中文名 | 类型 | 输出标志 | 说明 |
| --- | --- | --- | --- | --- |
| inst_type | 证券类型  | Int | N |  |
| symbol | 证券代码 | String | Y |  |
| name | 证券名称 | String | Y |  |
| list_date | 上市日期 | String | Y |  |
| delist_date | 退市日期 | String | N |  |
| status | 上市状态 | Int | N |  |
| currency | 货币 | String | N |  |
| buylot | 最小买入单位 | Int | N |  |
| selllot | 最大买入单位 | Int | N |  |
| pricetick | 最小变动单位 | Double | N |  |
| underlying | 对应标的 | String | N |  |
| product | 合约品种 | String | N |  |
| market | 交易所 | String | N |  |
| multiplier | 合约乘数 | Int | N |  |
  
## 交易日历（jz.secTradeCal）
 
交易日历
 
### 输入参数
 
| 字段 | 字段中文名 | 类型 | 输入标志 | 说明 |
| --- | --- | --- | --- | --- |
| start_date | 日期 | String | N |  |
| end_date | 日期 | String | N |  |
 
### 输出参数
 
| 字段 | 字段中文名 | 类型 | 输出标志 | 说明 |
| --- | --- | --- | --- | --- |
| trade_date | 日期 | String | Y |  |
| istradeday | 是否交易日 | String | Y |  |
| isweekday | 是否工作日 | String | N |  |
| isweekend | 是否周末 | String | N |  |
| isholiday | 是否节假日 | String | N |  |
  
## 指数成份股（lb.indexCons）
 
指数成份股
 
### 输入参数
 
| 字段 | 字段中文名 | 类型 | 输入标志 | 说明 |
| --- | --- | --- | --- | --- |
| index_code | 指数代码 | String | Y |  |
| start_date | 指定日期 | String | Y |  |
| end_date | 指定日期 | String | Y |  |
 
### 输出参数
 
| 字段 | 字段中文名 | 类型 | 输出标志 | 说明 |
| --- | --- | --- | --- | --- |
| index_code | 指数代码 | String | Y |  |
| symbol | 证券代码 | String | Y |  |
| in_date | 调入日期 | String | Y | 成份股在指数里的第一天 |
| out_date | 调出日期 | String | Y | 成份股在指数里的最后一天 |
  
## 指数基本信息（lb.indexInfo）
 
指数基本信息
 
### 输入参数
 
| 字段 | 字段中文名 | 类型 | 输入标志 | 说明 |
| --- | --- | --- | --- | --- |
| symbol | 证券代码 | String | N |  |
 
### 输出参数
 
| 字段 | 字段中文名 | 类型 | 输出标志 | 说明 |
| --- | --- | --- | --- | --- |
| symbol | 证券代码 | String | Y |  |
| name | 证券简称 | String | N |  |
| compname | 指数名称 | String | Y |  |
| exchmarket | 交易所 | String | Y | SH:上交所 SZ:深交所 |
  
## 行业代码表（lb.industryType）
 
行业代码表
 
### 输入参数
 
| 字段 | 字段中文名 | 类型 | 输入标志 | 说明 |
| --- | --- | --- | --- | --- |
| industry_src | 行业分类标准 | String | Y |  |
| level | 行业级别 | Int | Y |  |
| industry1_code | 一级行业代码 | String | N |  |
| industry2_code | 二级行业代码 | String | N |  |
| industry3_code | 三级行业代码 | String | N |  |
 
### 输出参数
 
| 字段 | 字段中文名 | 类型 | 输出标志 | 说明 |
| --- | --- | --- | --- | --- |
| industry_src | 行业分类标准 | String | Y |  |
| level | 行业级别 | Int | Y |  |
| industry_code | 行业代码 | String | N |  |
| industry_name | 行业名称 | String | Y |  |
| industry1_code | 一级行业代码 | String | Y |  |
| industry1_name | 一级行业名称 | String | N |  |
| industry2_code | 二级行业代码 | String | Y |  |
| industry2_name | 二级行业名称 | String | N |  |
| industry3_code | 三级行业代码 | String | Y |  |
| industry3_name | 三级行业名称 | String | N |  |
| industry4_code | 四级行业代码 | String | N |  |
| industry4_name | 四级行业名称 | String | N |  |
  
## 公募基金净值（lb.mfNav）
 
公募基金净值
 
### 输入参数
 
| 字段 | 字段中文名 | 类型 | 输入标志 | 说明 |
| --- | --- | --- | --- | --- |
| symbol | 证券代码 | String | N |  |
| start_date | 公告开始日期 | String | N |  |
| end_date | 公告结束日期 | String | N |  |
| start_pdate | 截止开始日期 | String | N |  |
| end_pdate | 截止结束日期 | String | N |  |
 
### 输出参数
 
| 字段 | 字段中文名 | 类型 | 输出标志 | 说明 |
| --- | --- | --- | --- | --- |
| symbol | 证券代码 | String | Y |  |
| ann_date | 公告日期 | String | Y |  |
| price_date | 截止日期 | String | Y |  |
| nav | 单位净值 | Double | Y |  |
| nav_accumulated | 累计净值 | Double | Y |  |
| netasset | 资产净值 | Double | N |  |
  
## 复权因子（lb.secAdjFactor）
 
复权因子
 
### 输入参数
 
| 字段 | 字段中文名 | 类型 | 输入标志 | 说明 |
| --- | --- | --- | --- | --- |
| symbol | 证券代码 | String | N |  |
| start_date | 开始日期 | String | N |  |
| end_date | 结束日期 | String | N |  |
 
### 输出参数
 
| 字段 | 字段中文名 | 类型 | 输出标志 | 说明 |
| --- | --- | --- | --- | --- |
| symbol | 证券代码 | String | Y |  |
| trade_date | 日期 | String | Y |  |
| adjust_factor | 复权因子 | Double | Y |  |
  
## 分红送股表（lb.secDividend）
 
分红送股
 
### 输入参数
 
| 字段 | 字段中文名 | 类型 | 输入标志 | 说明 |
| --- | --- | --- | --- | --- |
| symbol | 证券代码 | String | Y |  |
| start_date | 开始日期 | String | N | 除权除息日为筛选条件 |
| end_date | 结束日期 | String | N | 除权除息日为筛选条件 |
 
### 输出参数
 
| 字段 | 字段中文名 | 类型 | 输出标志 | 说明 |
| --- | --- | --- | --- | --- |
| symbol | 证券代码 | String | Y |  |
| div_enddate | 分红年度 | String | Y |  |
| ann_date | 预案公告日期 | String | Y |  |
| publish_date | 实施公告日期 | String | Y |  |
| record_date | 股权登记日 | String | Y |  |
| exdiv_date | 除权除息日 | String | Y |  |
| cash | 税前每股分红 | Double | Y |  |
| cash_tax | 税后每股分红 | Double | Y |  |
| share_ratio | 送股比例（每股） | Double | Y |  |
| share_trans_ratio | 转赠比例（每股） | Double | Y |  |
| cashpay_date | 派现日 | String | Y |  |
| bonus_list_date | 送股上市日 | String | Y |  |
  
## 行业分类（lb.secIndustry）
 
行业分类信息
 
### 输入参数
 
| 字段 | 字段中文名 | 类型 | 输入标志 | 说明 |
| --- | --- | --- | --- | --- |
| symbol | 证券代码 | String | N |  |
| industry_src | 行业分类来源 | String | Y |  |
| industry1_name | 一级行业名称 | String | N |  |
| industry2_name | 二级行业名称 | String | N |  |
| industry3_name | 三级行业名称 | String | N |  |
| industry4_name | 四级行业名称 | String | N |  |
| is_new | 是否最新 | String | N |  |
| trade_date | 交易日期 | String | N |  |
 
### 输出参数
 
| 字段 | 字段中文名 | 类型 | 输出标志 | 说明 |
| --- | --- | --- | --- | --- |
| symbol | 证券代码 | String | Y |  |
| industry_src | 行业分类来源 | String | Y |  |
| in_date | 纳入日期 | String | Y |  |
| out_date | 剔除日期 | String | Y |  |
| is_new | 是否最新 | String | N |  |
| industry1_code | 一级行业代码 | String | Y |  |
| industry2_code | 二级行业代码 | String | Y |  |
| industry3_code | 三级行业代码 | String | Y |  |
| industry4_code | 四级行业代码 | String | Y |  |
| industry1_name | 一级行业名称 | String | Y |  |
| industry2_name | 二级行业名称 | String | Y |  |
| industry3_name | 三级行业名称 | String | Y |  |
| industry4_name | 四级行业名称 | String | Y |  |
  
## 停复牌（lb.secSusp）
 
停复牌数据
 
### 输入参数
 
| 字段 | 字段中文名 | 类型 | 输入标志 | 说明 |
| --- | --- | --- | --- | --- |
| start_date | 停牌开始日期 | String | N |  |
| symbol | 证券代码 | String | N |  |
| end_date | 停牌结束日期 | String | N |  |
 
### 输出参数
 
| 字段 | 字段中文名 | 类型 | 输出标志 | 说明 |
| --- | --- | --- | --- | --- |
| symbol | 证券代码 | String | Y |  |
| ann_date | 停牌公告日期 | String | Y |  |
| susp_date | 停牌开始日期 | String | Y |  |
| susp_time | 停牌开始时间 | String | N |  |
| resu_date | 复牌日期 | String | Y |  |
| resu_time | 复牌时间 | String | N |  |
| susp_reason | 停牌原因 | String | Y |  |
 
