# Introduction 

DataCore是一款企业级开源量化数据系统，通过标准化接口提供高速实时行情、历史行情和参考数据等核心服务，覆盖股票、商品期货、股指期货、国债期货等品种，适配CTP、万得、聚源、Tushare等各类数据。
[](https://github.com/quantOS-org/DataCore/blob/master/doc/img/datacore.png)

# Features 

+ 统一的数据访问接口，接口简单易用
+ 快速适配新行情源
+ 跨市场、不同类型的行情数据格式统一
+ 适配多种参考数据源
+ 衍生数据计算服务框架
+ 支持多种编程语言API

# Architecture

[](https://github.com/quantOS-org/DataCore/blob/master/doc/img/architect.png)

## Mdlink
Mdlink是实时行情转发系统，接收由券商、期货交易所推送的行情，将其转为统一的数据格式供其他系统使用，使用C++语言开发。由接收程序和转发程序组成。

+ 接收程序包括mdlink_ctp和mdlink_tdf，分别用于接收ctp和tdf的行情
+ 转发程序 mdlink2，将多路行情汇总后统一转发出去。

需要接入新的行情源，只要开发新的mdlink接收程序即可。

## QMS
QMS缓存行情数据，生成分钟线，提供快照、分钟线查询服务。使用C++语言开发。

## DataServer
通过统一的DataAPI提供参考数据和行情数据服务。使用Scala语言开发。
适配实时行情源，提供行情订阅发布接口。
适配多种数参考数据源，包括Tushare、万得等数据源。

要使用DataServer，用户需要有相应的数据源。

+ 连接Mdlink和QMS，提供实时行情查询、订阅及日内分钟线查询服务。
+ 连接Tushare，提供历史行情数据查询服务。
+ 连接万得参考数据库，提供参考数据查询服务，用户也可以自己配置新的参考数据库。

# Download

请从这里下载编译好的可执行程序。[程序下载](https://github.com/quantOS-org/DataCore/blob/master/doc/download.md)

# Installation

参见[安装指南](https://github.com/quantOS-org/DataCore/blob/master/doc/install.md)

# API Reference

参见[API使用指南](https://github.com/quantOS-org/DataCore/blob/master/doc/api_ref.md)

# Contribute

欢迎参与开发！可以通过Pull Request的方式提交代码。

# Questions

如果您发现任何问题，请到[这里](https://github.com/quantOS-org/DataCore/issues/new)提交。

# License

Apache 2.0许可协议。版权所有(c)2017 quantOS-org.

