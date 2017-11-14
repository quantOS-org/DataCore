
/***********************************************************************

Copyright 2017 quantOS-org

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at:

http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.

***********************************************************************/
/////////////////////////////////////////////////////////////////////////
///@system 执行引擎系统
///@company 
///@file XMLAction.h
///@brief 定义了几个常用的XML操作类
///@history 
///20051231            创建该文件
/////////////////////////////////////////////////////////////////////////

#ifndef XMLACTION_H
#define XMLACTION_H

#include "CXML.h"

/////////////////////////////////////////////////////////////////////////
///CXMLDisplayAction是一个XML操作类，将指定的节点或者字符串输出到文件中
///@author    
///@version    1.0,20051231
/////////////////////////////////////////////////////////////////////////
class CXMLDisplayAction: public CXMLAction
{
public:
    ///构造函数
    ///@param    output    指定的输出文件
    CXMLDisplayAction(FILE *output=stdout);

    int nodeHandler(CXMLNode *pNode);
    int stringHandler(char *string);
private:
    FILE *m_output;
};

/////////////////////////////////////////////////////////////////////////
///CXMLListAction是一个XML操作类，将找到的节点或者字符串放到一个列表中
///@author    
///@version    1.0,20051231
/////////////////////////////////////////////////////////////////////////
class CXMLListAction: public CXMLAction
{
public:
    int nodeHandler(CXMLNode *pNode);
    int stringHandler(char *string);
    
    ///获取已经得到的字符串列表
    ///@return    已经得到的字符串列表
    vector<char *> *getStringList(void);
    
    ///获取已经得到的节点列表
    ///@return    已经得到的节点列表
    CXMLNodeList *getNodeList(void);
    
    ///清除已经得到的所有列表
    virtual void clearAll(void);
protected:
    vector<char *> stringList;
    CXMLNodeList nodeList;
};

/////////////////////////////////////////////////////////////////////////
///CXMLEqualAction是一个XML操作类，将检查所有相等的字符串或者节点
///@author    
///@version    1.0,20051231
/////////////////////////////////////////////////////////////////////////
class CXMLEqualCheckerAction: public CXMLListAction
{
public:
    int nodeHandler(CXMLNode *pNode);
    int stringHandler(char *string);

    ///判断两个字符串是否相等
    ///@param    string1    第一个字符串
    ///@param    string2    第二个字符串
    ///@return    1表示相等，0表示不相等
    virtual int isEqual(char *string1, char *string2);

    ///判断两个节点是否相等
    ///@param    pNode1    第一个节点
    ///@param    pNode2    第二个节点
    ///@return    1表示相等，0表示不相等
    virtual int isEqual(CXMLNode *pNode1, CXMLNode *pNode2);

    ///在发现重复字符串时的处理函数
    ///@param    string    重复的字符串
    ///@param    originalString    原来已经有的字符串
    ///@return    同stringHandler
    virtual int findDuplication(char *string, char *originalString);

    ///在发现重复节点时的处理函数
    ///@param    pNode    重复的节点
    ///@param    pOriginalNode    原来已经有的节点
    ///@return    同nodeHandler
    virtual int findDuplication(CXMLNode *pNode, CXMLNode *pOriginalNode);
};

///获取值问题
///群组函数问题
///迭置问题
///字符串检查问题
///多action合并为一个action问题
///设置属性问题
///否定条件

#endif
