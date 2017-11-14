#ifndef __TDF_API_H__
#define __TDF_API_H__

#include "TDFAPIStruct.h"
#include "TDFAPIVersion.h"

#if defined(WIN32) || defined(WIN64) || defined(_WINDOWS)
#ifdef TDF_API_EXPORT
#define TDFAPI __declspec(dllexport) 
#else	
#define TDFAPI __declspec(dllimport)
#endif
#else
#define TDFAPI __attribute((visibility("default")))
#endif

#ifdef __cplusplus
extern "C" {
#endif

enum TDF_ERR
{
    TDF_ERR_UNKOWN=-200,                // 未知错误

    TDF_ERR_INITIALIZE_FAILURE = -100,  // 初始化socket环境失败
    TDF_ERR_NETWORK_ERROR,              // 网络连接出现问题
    TDF_ERR_INVALID_PARAMS,             // 输入参数无效
    TDF_ERR_VERIFY_FAILURE,             // 登陆验证失败：原因为用户名或者密码错误；超出登陆数量
    TDF_ERR_NO_AUTHORIZED_MARKET,       // 所有请求的市场都没有授权
    TDF_ERR_NO_CODE_TABLE,              // 所有请求的市场该天都没有代码表
    
    TDF_ERR_SUCCESS = 0,                // 成功
};


//设置TDF环境变量值,在调用TDF_Open之前设置
//返回值：TDF_ERR_INVALID_PARAMS表示无效的nEnv，TDF_ERR_SUCCESS表示成功
TDFAPI int TDF_SetEnv(TDF_ENVIRON_SETTING nEnv, unsigned int nValue);


//同步函数，打开到TDFServer的连接，如果成功，则返回句柄，否则返回NULL，在TDF_Open期间发生了网络断开，将不会自动重连
//在调用期间，系统通知函数将收到MSG_SYS_CONNECT_RESULT，MSG_SYS_LOGIN_RESULT，MSG_SYS_CODETABLE_RESULT消息
//如果网络断开，则会收到MSG_SYS_DISCONNECT_NETWORK，pErr中存放错误代码，只有在错误代码为 TDF_ERR_NETWORK_ERROR 时候，外部才应该做重连逻辑
TDFAPI THANDLE TDF_Open(TDF_OPEN_SETTING* pSettings, TDF_ERR* pErr);
//可以配置多组服务器，取多组服务器中最快的行情推送
TDFAPI THANDLE TDF_OpenExt(TDF_OPEN_SETTING_EXT* pSettings,TDF_ERR* pErr);
//通过代理打开连接，回调消息和错误代码和TDF_Open一样
TDFAPI THANDLE TDF_OpenProxy(TDF_OPEN_SETTING* pOpenSettings, TDF_PROXY_SETTING* pProxySettings, TDF_ERR* pErr);
TDFAPI THANDLE TDF_OpenProxyExt(TDF_OPEN_SETTING_EXT* pOpenSettings, TDF_PROXY_SETTING* pProxySettings, TDF_ERR* pErr);

//获取指定市场的代码表，在已经收到MSG_SYS_CODETABLE_RESULT 消息之后，可以获得代码表
//获取到的代码表，需要调用TDF_FreeArr来释放内存
TDFAPI int TDF_GetCodeTable(THANDLE hTdf, const char* szMarket, TDF_CODE** pCode, unsigned int* pItems);

// 从万得代码来获取详细的期权代码信息
// pCodeInfo指针由用户提供，
// 如果成功获取，则返回TDF_ERR_SUCCESS，否则返回 TDF_ERR_NO_CODE_TABLE 或 TDF_ERR_INVALID_PARAMS

TDFAPI int TDF_GetOptionCodeInfo(THANDLE hTdf, const char* szWindCode, TDF_OPTION_CODE* pCodeInfo);

//同步函数，关闭连接，不要再回调函数里面调用，否则会卡死
TDFAPI int TDF_Close(THANDLE hTdf);

TDFAPI void TDF_FreeArr(void *pArr);

//登陆后订阅; 此函数是个异步函数，最好在TDF_Open成功之后调用
TDFAPI int TDF_SetSubscription(THANDLE hTdf, const char* szSubScriptions, SUBSCRIPTION_STYLE nSubStyle);

#ifdef __cplusplus
}
#endif

#endif