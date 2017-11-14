
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
///@brief 实现了几个常用的XML操作类
///@history 
///20051231            创建该文件
/////////////////////////////////////////////////////////////////////////

#include "XMLAction.h"

/////////////////////////////////////////////////////////////////////////
///CXMLDisplayAction的实现
/////////////////////////////////////////////////////////////////////////

CXMLDisplayAction::CXMLDisplayAction(FILE *output)
{
    m_output=output;
}

int CXMLDisplayAction::nodeHandler(CXMLNode *pNode)
{
    pNode->write(m_output);
    return 1;
}

int CXMLDisplayAction::stringHandler(char *string)
{
    fprintf(m_output,"%s\n",string);
    return 1;
}

/////////////////////////////////////////////////////////////////////////
///CXMLListAction的实现
/////////////////////////////////////////////////////////////////////////

int CXMLListAction::nodeHandler(CXMLNode *pNode)
{
    nodeList.push_back(pNode);
    return 1;
}

int CXMLListAction::stringHandler(char *string)
{
    stringList.push_back(string);
    return 1;
}
    
vector<char *> *CXMLListAction::getStringList(void)
{
    return &stringList;
}
    
CXMLNodeList *CXMLListAction::getNodeList(void)
{
    return &nodeList;
}

void CXMLListAction::clearAll(void)
{
    nodeList.clear();
    stringList.clear();
}

/////////////////////////////////////////////////////////////////////////
///CXMLEqualCheckerAction的实现
/////////////////////////////////////////////////////////////////////////

int CXMLEqualCheckerAction::nodeHandler(CXMLNode *pNode)
{
    CXMLNodeList::iterator it;
    
    for (it=nodeList.begin();it<nodeList.end();it++)
    {
        if (isEqual(pNode,*it))
        {
            return findDuplication(pNode,*it);
        }
    }
    return CXMLListAction::nodeHandler(pNode);
}

int CXMLEqualCheckerAction::stringHandler(char *string)
{
    vector<char *>::iterator it;
    
    for (it=stringList.begin();it<stringList.end();it++)
    {
        if (isEqual(string,*it))
        {
            return findDuplication(string,*it);
        }
    }
    return CXMLListAction::stringHandler(string);
}

int CXMLEqualCheckerAction::isEqual(char *string1, char *string2)
{
    if (!strcmp(string1,string2))
    {
        return 1;
    }
    else
    {
        return 0;
    }
}

int CXMLEqualCheckerAction::isEqual(CXMLNode *pNode1, CXMLNode *pNode2)
{
    if (pNode1==pNode2)
    {
        return 1;
    }
    else
    {
        return 0;
    }
}

int CXMLEqualCheckerAction::findDuplication(char *string, char *originalString)
{
    return 1;
}

int CXMLEqualCheckerAction::findDuplication(CXMLNode *pNode, CXMLNode *pOriginalNode)
{
    return 1;
}
