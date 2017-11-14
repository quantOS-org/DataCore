#ifndef __TDF_API_STRUCT_H__ 
#define __TDF_API_STRUCT_H__
#pragma  pack(push)
#pragma pack(1)

#ifndef THANDLE
typedef void* THANDLE;
#endif

#ifndef __int64
#define __int64 long long
#endif
#define MAXSERVER 4

/******************************************************************* 消息头 *********************************************************************/
struct TDF_APP_HEAD
{
	int	nHeadSize;         //本记录结构大小
	int	nItemCount;	       //记录条数
	int	nItemSize;         //记录大小
};
struct TDF_MSG
{
	unsigned short  	    sFlags;		        //16位 标识符.
	int  	                nDataType;	        //16位 数据类型 见enum TDF_MSG_ID
	int			            nDataLen;	        //32位 数据长度（不包括TDF_APP_HEAD的长度）
	int			            nServerTime;		//32位服务器时间戳（精确到毫秒HHMMSSmmm），对于原生压缩，就是本地时间
	int     		        nOrder;		        //32位 流水号
	int                     nConnectId;         //连接ID，在TDF_Open是，配置里面设定的值
	TDF_APP_HEAD*           pAppHead;	        //应用头
	void*                   pData;              //数据指针
};
/*******************************************************************  回调函数 *********************************************************************/
//数据回调，用于通知用户收到了行情、逐笔成交，逐笔委托，委托队列等,pMsgHead->pAppHead->ItemCount字段可以获知得到了多少条记录，pMsgHead->pAppHead->pData指向第一条数据记录
typedef void (*TDF_DataMsgHandler)  (THANDLE hTdf, TDF_MSG* pMsgHead);    
//系统消息回调，用于通知用户收到了网络断开事件、连接（重连）结果、代码表结果等。当获取系统消息时，pMsgHead->pAppHead指针为空, pMsgHead->pData指向相应的结构体
typedef void (*TDF_SystemMsgHandler)(THANDLE hTdf, TDF_MSG* pMsgHead);  

/******************************************************************* 消息类型定义 *********************************************************************/
//消息类型对应的结构体见后面
enum TDF_MSG_ID
{
    MSG_INVALID = -100,
    //系统消息
    MSG_SYS_DISCONNECT_NETWORK,     //网络断开事件, 相应的结构体为NULL
    MSG_SYS_CONNECT_RESULT,         //主动发起连接的结果
    MSG_SYS_LOGIN_RESULT,           //登陆应答
    MSG_SYS_CODETABLE_RESULT,       //获取代码表结果
    MSG_SYS_QUOTATIONDATE_CHANGE,   //行情日期变更通知
    MSG_SYS_MARKET_CLOSE,           //闭市
    MSG_SYS_HEART_BEAT,             //服务器心跳消息, 相应的结构体为NULL
    //数据消息
    MSG_DATA_INDEX,                 //指数数据
    MSG_DATA_MARKET,                //行情数据
    MSG_DATA_FUTURE,                //期货行情
    MSG_DATA_TRANSACTION,           //逐笔成交
    MSG_DATA_ORDERQUEUE,            //委托队列
    MSG_DATA_ORDER,                 //逐笔委托
	MSG_DATA_BBQTRANSACTION,        //BBQ现券成交数据
	MSG_DATA_BBQBID,                //BBQ现券报价数据

	MSG_SYS_PACK_OVER   = -10,              //当前网络包解析完毕
};

/******************************************************************* 系统消息 *********************************************************************/
//MSG_SYS_PACK_OVER
struct TDF_PACK_OVER
{
	int nDataNumber;
	int nConID;
};
//数据类型MSG_SYS_CONNECT_RESULT
struct TDF_CONNECT_RESULT
{
    char szIp[32];
    char szPort[8];
    char szUser[64];
    char szPwd[64];

    unsigned int nConnResult; //为0则表示连接失败，非0则表示连接成功
    int nConnectionID;        //连接ID
};
//数据类型MSG_SYS_LOGIN_RESULT
struct TDF_LOGIN_RESULT
{
    unsigned int nLoginResult;//为0则表示登陆验证失败，非0则表示验证成功

    char szInfo[256];       //登陆结果文本
    int nMarkets;           //市场个数
    char szMarket[256][8];  //市场代码 SZ, SH, CF, SHF, CZC, DCE
    int nDynDate[256];      //动态数据日期
};
//数据类型MSG_SYS_CODETABLE_RESULT
struct TDF_CODE_RESULT
{
    char szInfo[128];       //代码表结果文本
    int nMarkets;           //市场个数
    char szMarket[256][8];  //市场代码
    int nCodeCount[256];    //代码表项数
    int nCodeDate[256];     //代码表日期

};
//数据类型MSG_SYS_QUOTATIONDATE_CHANGE
struct TDF_QUOTATIONDATE_CHANGE
{
    char szMarket[8];	    //市场代码
    int nOldDate;	        //原行情日期
    int nNewDate;	        //新行情日期
};
//数据类型MSG_SYS_MARKET_CLOSE
struct TDF_MARKET_CLOSE
{
    char    szMarket[8];        //交易所名称
    int		nTime;				//时间(HHMMSSmmm)
    char	chInfo[64];			//闭市信息
};

/******************************************************************* 代码表 *********************************************************************/
//使用TDF_GetCodeTable获取基本代码表，使用TDF_GetOptionCodeInfo获取期权代码表（期权含额外信息）
struct TDF_CODE
{
    char szWindCode[32];    //Wind Code: AG1302.SHF
    char szMarket[8];       //market code: SHF
    char szCode[32];        //original code:ag1302
    char szENName[32];
    char szCNName[32];      //chinese name: 沪银1302
    int nType;                            
};
struct TDF_OPTION_CODE
{
    TDF_CODE basicCode;
    
    char szContractID[32];// 期权合约代码
    char szUnderlyingSecurityID[32];//// 标的证券代码
    char chCallOrPut;               // 认购认沽C1        认购，则本字段为“C”；若为认沽，则本字段为“P”
    int  nExerciseDate;             // 期权行权日，YYYYMMDD    
    //扩充字段
    char chUnderlyingType;			// 标的证券类型C3    0-A股 1-ETF (EBS – ETF， ASH – A 股)
	char chOptionType;              // 欧式美式C1        若为欧式期权，则本字段为“E”；若为美式期权，则本字段为“A”
	
	char chPriceLimitType;          // 涨跌幅限制类型C1 ‘N’表示有涨跌幅限制类型, ‘R’表示无涨跌幅限制类型
	int  nContractMultiplierUnit;	// 合约单位,         经过除权除息调整后的合约单位, 一定是整数
	int  nExercisePrice;            // 期权行权价,       经过除权除息调整后的期权行权价，右对齐，精确到厘
	int  nStartDate;                // 期权首个交易日,YYYYMMDD
	int  nEndDate;                  // 期权最后交易日/行权日，YYYYMMDD
	int  nExpireDate;               // 期权到期日，YYYYMMDD
};
union TD_EXCODE_INFO
{
	struct TD_OptionCodeInfo            //futures options 专用 (nType >= 0x90 && nType <= 0x95),非期权下列字段无效
	{
		char chContractID[32];           // 期权合约代码C19
		char szUnderlyingSecurityID[32]; // 标的证券代码
		char chUnderlyingType;			 // 标的证券类型C3    0-A股 1-ETF (EBS – ETF， ASH – A 股)
		char chOptionType;               // 欧式美式C1        若为欧式期权，则本字段为“E”；若为美式期权，则本字段为“A”
		char chCallOrPut;                // 认购认沽C1        认购，则本字段为“C”；若为认沽，则本字段为“P”
		char chPriceLimitType;           // 涨跌幅限制类型C1 ‘N’表示有涨跌幅限制类型, ‘R’表示无涨跌幅限制类型
		int  nContractMultiplierUnit;	 // 合约单位,         经过除权除息调整后的合约单位, 一定是整数
		int  nExercisePrice;             // 期权行权价,       经过除权除息调整后的期权行权价，右对齐，精确到厘
		int  nStartDate;                 // 期权首个交易日,YYYYMMDD
		int  nEndDate;                   // 期权最后交易日/行权日，YYYYMMDD
		int  nExerciseDate;              // 期权行权日，YYYYMMDD
		int  nExpireDate;                // 期权到期日，YYYYMMDD
	}Option;
};
struct TDF_CODE_INFO
{
	int  nMarketID;           //交易所ID,定义如下
	int  nDataLevel;          //数据Level
	int  nFlag;               //数据来源
	int  nType;					//证券类型
	char chCode[32];            //证券代码
	char chName[64];			//汉语证券名称
	TD_EXCODE_INFO exCodeInfo;
};

//交易所ID定义///////////////////////////////
#ifndef ID_MARKET_DEFINE
#define ID_MARKET_DEFINE
#define ID_MARKET_SZ	0	//深圳
#define ID_MARKET_SH	1	//上海
#define ID_MARKET_PT	2	//PT
#define ID_MARKET_HZ	3	//板块指数
#define ID_MARKET_SL	4	//自定义市场
#define ID_MARKET_MI	5	//自定义指数
#define ID_MARKET_HK	6	//港股
#define ID_MARKET_QH	7	//期货
#define ID_MARKET_FE	8	//外汇
#define ID_MARKET_KF	9	//外汇(港股)
#define ID_MARKET_CF	10	//中国金融指数

#define ID_MARKET_SHF	11	//上海金属商品交易所
#define ID_MARKET_CZC	12	//郑州商品
#define ID_MARKET_DCE	13	//大连商品
/////////////////////////////////////////////
#define ID_MARKET_CME	14  //芝加哥商业交易所
#define ID_MARKET_CBT	15  //芝加哥期货交易所
#define ID_MARKET_CMX	16  //纽约商品期货交易所
#define ID_MARKET_NYM	17  //纽约商品交易所
#define ID_MARKET_NYB	18  //纽约期货交易所
#define ID_MARKET_IPE	19	//伦敦国际石油交易所
#define ID_MARKET_LME	20	//伦敦金属交易所
#define ID_MARKET_N		21  //纽约交易所
#define ID_MARKET_O		22  //纳斯达克交易所
#define ID_MARKET_HKF	23  //香港交易所期货/期权
/////////////////////////////////////////////
#endif
/******************************************************************* 数据消息 *********************************************************************/
//数据类型MSG_DATA_BBQTRANSACTION,     
struct TDF_BBQTRANSACTION_DATA
{
	char        szWindCode[32];         //600001.SH 
	int         nActionDay;             //业务发生日(自然日)
	int			nTime;					//时间(HHMMSSmmm)
	int         nDoneID;                //成交报价编号
	int			nPrice;				    //成交收益率(%)或价格 *10000
	char        chPriceStatus;          //收益率或价格标识	1：收益率 	2：价格
	char        chStatus;               //收益率或价格标识	1：收益率 	2：价格
	char        chDirection;            //成交方向	1：done	2：gvn		3：tkn		4：其他
	char        chSource;               //报价机构	1：国际 2：平安利顺 3：中诚 4：天津信唐	5：国利
	char        chSpecialFlag;          //标识报价是否是行权或到期	0无标记	1备注中包含到期		2备注中包含行权
};
//数据类MSG_DATA_BBQBID,    
struct TDF_BBQBID_DATA
{
	char        szWindCode[32];         //600001.SH 
	int         nActionDay;             //业务发生日(自然日)
	int			nTime;					//时间(HHMMSSmmm)
	char        chSource;               //报价机构

	int         nBidPrice;		       //买入收益率或价格 x10000
	__int64         nBidVolume;		   //买入量 x10000
	char        chBidPriceStatus;      //收益率或价格标识	1：收益率 	2：价格
	char        chIsBid;               //是否bid	0：不是bid（普通报价）	1：是bid（意向报价）
	char        chBidSpecialFlag;      //标识报价是否是行权或到期	0无标记	1备注中包含到期		2备注中包含行权
	char        chBidStatus;           //买入最优报价状态	0：正常报价	1：撤销报价

	int         nOfrPrice;		       //卖出收益率或价格 x10000
	__int64         nOfrVolume;		   //卖出量	所有量的加总，如2000+1000，就合并为3000；如字符串中含- - 则视为0. x10000
	char        chOfrPriceStatus;      //收益率或价格标识	1：收益率 	2：价格
	char        chIsOfr;               //是否ofr	0：不是ofr（普通报价）	  1：是ofr（意向报价）
	char        chOfrSpecialFlag;      //标识报价是否是行权或到期	0无标记	1备注中包含到期		2备注中包含行权
	char        chOfrStatus;           //卖出最优报价状态	0：正常报价	1：撤销报价
};
//数据类型MSG_DATA_MARKET
struct TDF_MARKET_DATA
{
    char        szWindCode[32];         //600001.SH 
    char        szCode[32];             //原始Code
    int         nActionDay;             //业务发生日(自然日)
    int         nTradingDay;            //交易日
    int			 nTime;					//时间(HHMMSSmmm)
    int			 nStatus;				//状态
    unsigned int nPreClose;				//前收盘价
    unsigned int nOpen;					//开盘价
    unsigned int nHigh;					//最高价
    unsigned int nLow;					//最低价
    unsigned int nMatch;				//最新价
    unsigned int nAskPrice[10];			//申卖价
    unsigned int nAskVol[10];			//申卖量
    unsigned int nBidPrice[10];			//申买价
    unsigned int nBidVol[10];			//申买量
    unsigned int nNumTrades;			//成交笔数
    __int64		 iVolume;				//成交总量
    __int64		 iTurnover;				//成交总金额
    __int64		 nTotalBidVol;			//委托买入总量
    __int64		 nTotalAskVol;			//委托卖出总量
    unsigned int nWeightedAvgBidPrice;	//加权平均委买价格
    unsigned int nWeightedAvgAskPrice;  //加权平均委卖价格
    int			 nIOPV;					//IOPV净值估值
    int			 nYieldToMaturity;		//到期收益率
    unsigned int nHighLimited;			//涨停价
    unsigned int nLowLimited;			//跌停价
    char		 chPrefix[4];			//证券信息前缀
    int			 nSyl1;					//市盈率1
    int			 nSyl2;					//市盈率2
    int			 nSD2;					//升跌2（对比上一笔）
	const TDF_CODE_INFO *  pCodeInfo;     //代码信息， TDF_Close，清盘重连后，此指针无效
};
//数据类型MSG_DATA_INDEX
struct TDF_INDEX_DATA
{
    char        szWindCode[32];         //600001.SH 
    char        szCode[32];             //原始Code
    int         nActionDay;             //业务发生日(自然日)
    int         nTradingDay;            //交易日
    int         nTime;			        //时间(HHMMSSmmm)
    int		    nOpenIndex;		        //今开盘指数
    int 	    nHighIndex;		        //最高指数
    int 	    nLowIndex;		        //最低指数
    int 	    nLastIndex;		        //最新指数
    __int64	    iTotalVolume;	        //参与计算相应指数的交易数量
    __int64	    iTurnover;		        //参与计算相应指数的成交金额
    int		    nPreCloseIndex;	        //前盘指数
	const TDF_CODE_INFO *  pCodeInfo;   //代码信息， TDF_Close，清盘重连后，此指针无效
};
//数据类型MSG_DATA_FUTURE
struct TDF_FUTURE_DATA
{
    char        szWindCode[32];         //600001.SH 
    char        szCode[32];             //原始Code
    int         nActionDay;             //业务发生日(自然日)
    int         nTradingDay;            //交易日
    int			 nTime;					//时间(HHMMSSmmm)	
    int			 nStatus;				//状态
    __int64		 iPreOpenInterest;		//昨持仓
    unsigned int nPreClose;				//昨收盘价
    unsigned int nPreSettlePrice;		//昨结算
    unsigned int nOpen;					//开盘价	
    unsigned int nHigh;					//最高价
    unsigned int nLow;					//最低价
    unsigned int nMatch;				//最新价
    __int64		 iVolume;				//成交总量
    __int64		 iTurnover;				//成交总金额
    __int64		 iOpenInterest;			//持仓总量
    unsigned int nClose;				//今收盘
    unsigned int nSettlePrice;			//今结算
    unsigned int nHighLimited;			//涨停价
    unsigned int nLowLimited;			//跌停价
    int			 nPreDelta;			    //昨虚实度
    int			 nCurrDelta;            //今虚实度
    unsigned int nAskPrice[5];			//申卖价
    unsigned int nAskVol[5];			//申卖量
    unsigned int nBidPrice[5];			//申买价
    unsigned int nBidVol[5];			//申买量
	//Add 20140605
	int	nAuctionPrice;		            //波动性中断参考价
	int	nAuctionQty;		            //波动性中断集合竞价虚拟匹配量	
	int nAvgPrice;                      //郑商所期货均价
	const TDF_CODE_INFO *  pCodeInfo;   //代码信息， TDF_Close，清盘重连后，此指针无效
};
//数据类型MSG_DATA_TRANSACTION
struct TDF_TRANSACTION
{
    char    szWindCode[32];             //600001.SH 
    char    szCode[32];                 //原始Code
    int     nActionDay;                 //自然日
    int 	nTime;		                //成交时间(HHMMSSmmm)
    int 	nIndex;		                //成交编号
    int		nPrice;		                //成交价格
    int 	nVolume;	                //成交数量
    int		nTurnover;	                //成交金额
    int     nBSFlag;                    //买卖方向(买：'B', 卖：'S', 不明：' ')
    char    chOrderKind;                //成交类别
    char    chFunctionCode;             //成交代码
    int	    nAskOrder;	                //叫卖方委托序号
    int	    nBidOrder;	                //叫买方委托序号
	const TDF_CODE_INFO *  pCodeInfo;   //代码信息， TDF_Close，清盘重连后，此指针无效
};
//数据类型MSG_DATA_ORDER
struct TDF_ORDER
{
	char    szWindCode[32]; //600001.SH 
	char    szCode[32];     //原始Code
	int 	nActionDay;	    //委托日期(YYMMDD)
	int 	nTime;			//委托时间(HHMMSSmmm)
	int 	nOrder;	        //委托号
	int		nPrice;			//委托价格
	int 	nVolume;		//委托数量
	char    chOrderKind;	//委托类别
	char    chFunctionCode;	//委托代码('B','S','C')
	const TDF_CODE_INFO *  pCodeInfo;     //代码信息， TDF_Close，清盘重连后，此指针无效
};
//数据类型MSG_DATA_ORDERQUEUE
struct TDF_ORDER_QUEUE
{
	char    szWindCode[32]; //600001.SH 
	char    szCode[32];     //原始Code
	int     nActionDay;     //自然日
	int 	nTime;			//时间(HHMMSSmmm)
	int     nSide;			//买卖方向('B':Bid 'A':Ask)
	int		nPrice;			//委托价格
	int 	nOrders;		//订单数量
	int 	nABItems;		//明细个数
	int 	nABVolume[200];	//订单明细
	const TDF_CODE_INFO *  pCodeInfo;     //代码信息， TDF_Close，清盘重连后，此指针无效
};
/******************************************************************* 订阅相关 *********************************************************************/
enum DATA_TYPE_FLAG
{
	DATA_TYPE_TRANSACTION = 0x2,           //逐笔成交
	DATA_TYPE_ORDER       = 0x4,           //逐笔委托
	DATA_TYPE_ORDERQUEUE  = 0x8,           //委托队列 
	DATA_TYPE_NONE = 0,                    //只要行情，其他都不订阅；订阅多个类型DATA_TYPE_TRANSACTION | DATA_TYPE_ORDER | DATA_TYPE_ORDERQUEUE
};
enum SUBSCRIPTION_STYLE
{
	SUBSCRIPTION_FULL = 3,                //全市场订阅
	SUBSCRIPTION_SET=0,                   //设置为订阅列表中股票，
	SUBSCRIPTION_ADD=1,                   //增加订阅列表中股票
	SUBSCRIPTION_DEL=2,                   //删除列表中的订阅
};

/******************************************************************* 配置 *********************************************************************/
//环境设置，在调用TDF_Open之前设置k
enum TDF_ENVIRON_SETTING
{
	TDF_ENVIRON_HEART_BEAT_INTERVAL,       //Heart Beat间隔（秒数）, 若值为0则表示默认值10秒钟
	TDF_ENVIRON_MISSED_BEART_COUNT,        //如果没有收到心跳次数超过这个值，且没收到其他任何数据，则判断为掉线，若值0为默认次数2次
	TDF_ENVIRON_OPEN_TIME_OUT,             //在调TDF_Open期间，接收每一个数据包的超时时间（秒数，不是TDF_Open调用总的最大等待时间），若值为0则默认30秒
	TDF_ENVIRON_USE_PACK_OVER,
};
//单服务器配置（可用多服务器实现，保留单服务器配置以兼容旧API使用方法）
//相比旧API，少了：nReconnectCount、nReconnectGap、nProtocol，nDate
struct TDF_OPEN_SETTING
{
    char szIp[32];
    char szPort[8];
    char szUser[64];
    char szPwd[64];
	//回调函数设置
	TDF_DataMsgHandler pfnMsgHandler;       //数据消息处理回调
	TDF_SystemMsgHandler pfnSysMsgNotify;   //系统消息通知回调

	//unsigned int nProtocol;                 //协议号，为0则为默认，或者0x6001
	//订阅设置
	const char* szMarkets;                  // 市场订阅！例如"SZ;SH;CF;SHF;DCE;SHF"，需要订阅的市场列表，以“;”分割,为空则订阅所有支持的市场
	const char* szSubScriptions;            // 代码订阅！例如"600000.sh;ag.shf;000001.sz"，需要订阅的股票，以“;”分割，为空则订阅全市场
	unsigned int nTypeFlags;                // 数据类型订阅！支持订阅3种类型TRANSACTION;ORDER;ORDERQUEUE。 ！注意：行情数据任何时候都发送，不需要订阅! 参见enum DATA_TYPE_FLAG

	unsigned int nTime;                     //为0则请求实时行情，为0xffffffff从头请求
	unsigned int nConnectionID;             //连接ID，连接回调消息的附加结构 TDF_CONNECT_RESULT中 会包含这个ID
};

//多服务器配置
struct TDF_SERVER_INFO
{
	char szIp[32];
	char szPort[8];
	char szUser[64]; //TODO
	char szPwd[64];
};
struct TDF_OPEN_SETTING_EXT
{
	//服务器设置
	TDF_SERVER_INFO	siServer[MAXSERVER];
	unsigned int nServerNum; 
	//回调函数设置
	TDF_DataMsgHandler pfnMsgHandler;       //数据消息处理回调
	TDF_SystemMsgHandler pfnSysMsgNotify;   //系统消息通知回调
	
	//unsigned int nProtocol;                 //协议号，为0则为默认，或者0x6001
	//订阅设置
	const char* szMarkets;                  // 市场订阅！例如"SZ;SH;CF;SHF;DCE;SHF"，需要订阅的市场列表，以“;”分割,为空则订阅所有支持的市场
	const char* szSubScriptions;            // 代码订阅！例如"600000.sh;ag.shf;000001.sz"，需要订阅的股票，以“;”分割，为空则订阅全市场
	unsigned int nTypeFlags;                // 数据类型订阅！支持订阅3种类型TRANSACTION;ORDER;ORDERQUEUE。 ！注意：行情数据任何时候都发送，不需要订阅! 参见enum DATA_TYPE_FLAG

	unsigned int nTime;                     //为0则请求实时行情，为0xffffffff从头请求
	unsigned int nConnectionID;             //连接ID，连接回调消息的附加结构 TDF_CONNECT_RESULT中 会包含这个ID
};
//代理服务器设置
enum TDF_PROXY_TYPE
{
	TDF_PROXY_SOCK4,
	TDF_PROXY_SOCK4A,
	TDF_PROXY_SOCK5,
	TDF_PROXY_HTTP11,
};
struct TDF_PROXY_SETTING
{
	TDF_PROXY_TYPE nProxyType;
	char szProxyHostIp[32];
	char szProxyPort[8];
	char szProxyUser[32];
	char szProxyPwd[32];
};

#pragma pack(pop)
#endif
