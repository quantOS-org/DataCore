
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
///@file CXML.h
///@brief定义了与XML操作有关的类接口
///@history 
///20051011            创建该文件
/////////////////////////////////////////////////////////////////////////

#ifndef CXML_H
#define CXML_H

//#include "CBaseObject.h"
#include <vector>
#include <stack>
#include <cstring>
#include <cstdio>
#include <stdlib.h>
#include "base/Logger.h"
using namespace std;
using namespace jzs;

///安全的字符串指针类
class CStringPtr
{
private:
    const char *m_ptr;
public:
    CStringPtr(const char *ptr=NULL)
    {
        if (ptr!=NULL)
        {
            m_ptr=strdup(ptr);
        }
        else
        {
            m_ptr=NULL;
        }
    }
    CStringPtr(const CStringPtr &ptr)
    {
        if (ptr.m_ptr!=NULL)
        {
            m_ptr=strdup(ptr.m_ptr);
        }
        else
        {
            m_ptr=NULL;
        }
    }
    const CStringPtr & operator = (const CStringPtr &ptr)
    {
        if (m_ptr!=NULL)
        {
            free((void*)m_ptr);
        }
        if (ptr.m_ptr!=NULL)
        {
            m_ptr=strdup(ptr.m_ptr);
        }
        else
        {
            m_ptr=NULL;
        }
        return *this;
    }
    ~CStringPtr(void)
    {
        if (m_ptr!=NULL)
        {
            free((void*)m_ptr);
        }
    }
    const char *operator =(const char *ptr)
    {
        if (m_ptr!=NULL)
        {
            free((void*)m_ptr);
        }
        if (ptr==NULL)
        {
            m_ptr=NULL;
        }
        else
        {
            m_ptr=strdup(ptr);
        }
        return m_ptr;
    }
    operator const char *(void)
    {
        if (m_ptr==NULL)
        {
            return "";
        }
        return m_ptr;
    }
};

/////////////////////////////////////////////////////////////////////////
///CXMLProperty是一个XML标记中的一个属性，由名称和值组成，如果只有名称，
///则value为NULL。对于像doctype等标记，不使用等号来分隔，那么后面的每一项
///将都被认为是一个只有名称的属性。未来还需要考虑namespace的情况
///@author    
///@version    2.0,20051011
/////////////////////////////////////////////////////////////////////////
class CXMLProperty//: public CBaseObject
{
public:
    ///构造函数，设置初始的名称和值，这里的内存将不使用外部申请的内存
    ///@param    name    名称
    ///@param    value    值
    CXMLProperty(const char *name=NULL, const char *value=NULL);
    
    ///析构函数，不释放名称和值的空间
    ~CXMLProperty(void);

    virtual int isA(const char *objectType);
    virtual const char *getType(void);
    //virtual void output(Logger *pLogger,int indent=0);
    
    ///复制一份自己
    ///@return    复制产生的新节点
    CXMLProperty *clone(void);

    ///设置名称
    ///@param    name    名称
    void setName(const char *name);
    
    ///设置值
    ///@param    value    值
    void setValue(const char *value);
    
    ///获得名称
    ///@return    名称
    const char *getName(void);
    
    ///获得获得值
    ///@return    值
    const char *getValue(void);
    
    ///输出到文件中，会自动进行必要的字符转换
    ///@param    output    指定的文件
    ///@return    1表示成功，0表示失败
    int write(FILE *output);
    int write(char *output);
private:
    ///名称
    CStringPtr m_name;
    
    ///值
    CStringPtr m_value;
};

/////////////////////////////////////////////////////////////////////////
///CXMLPropertyList是一个属性列表，用于存储一个XML标记中的所有属性。在本
///实现中，没有检查属性重名问题，因为这个属性列表会同时应用在DTD定义等特殊
///标记中，而在这些特殊标记中，属性重名是合法的。未来的实现可以通过参数
///区分可以和不可以重名的属性列表
///@author    
///@version    2.0,20051011
/////////////////////////////////////////////////////////////////////////
class CXMLPropertyList: public vector<CXMLProperty *>
{
public:
    ///构造函数，创建一个空的属性列表
    CXMLPropertyList(void);
    
    ///析构函数，释放本身的空间和包含的所有CXMLProperty
    virtual ~CXMLPropertyList(void);

    virtual int isA(const char *objectType);
    virtual const char *getType(void);
    //virtual void output(Logger *pLogger,int indent=0);

    ///复制一份自己
    ///@return    复制产生的新节点
    CXMLPropertyList *clone(void);

    ///增加一个属性
    ///@param    name    名称
    ///@param    value    值
    ///@return    1表示成功，0表示失败
    int addProperty(const char *name, const char *value);
    
    ///增加一个属性
    ///@param    name    名称
    ///@param    value    值
    ///@return    1表示成功，0表示失败
    int addProperty(const char *name, int value);

    ///找到某个属性对应的编号，如果有多个属性都是这个名称，则找到第一个
    ///@param    name    属性名称
    ///@return    对应的编号，如果没有找到，则返回-1
    int findID(const char *name);

    ///删除一个指定名称的属性，如果有多个属性都是这个名称，则删除第一个
    ///@param    name    名称
    ///@return    1表示成功，0表示失败
    int removeProperty(const char *name);
    
    ///替换一个指定名称的属性的值，如果有多个属性都是这个名称，则替换第一个
    ///@param    name    名称
    ///@param    newValue    新的值
    ///@return    1表示成功，0表示失败
    int replaceProperty(const char *name, const char *newValue);
    
    ///根据名称，寻找一个属性，如果有多个属性都是这个名称，则只找第一个
    ///@param    name    名称
    ///@return    找到的属性
    CXMLProperty *findProperty(const char *name);
    
    ///根据名称，寻找一个属性的值，如果有多个属性都是这个名称，则只找第一个
    ///@param    name    名称
    ///@return    找到的属性值，没有该属性，则返回NULL，如果该属性没有值，则返回""
    const char *findValue(const char *name);
    
    ///输出到文件中，每个属性前加一个空格
    ///@param    output    指定的文件
    ///@return    1表示成功，0表示失败
    int write(FILE *output);
    int write(char *output);
};

/////////////////////////////////////////////////////////////////////////
///XMLElementType是一个XML元素类型的枚举值类型
///@author    
///@version    2.0,20051011
/////////////////////////////////////////////////////////////////////////
typedef enum
{
    XMLNotElement,            ///不是XML元素
    XMLStartElement,        ///开始元素，例如<user ...>
    XMLEndElement,            ///结束元素，例如</user>
    XMLSingleElement,        ///单独元素，例如<user/>
    XMLTextElement,            ///文字元素，例如Hello world
    XMLCommentElement,        ///注释元素，例如<!-- This is comment -->
    XMLDocumentElement,        ///文档说明元素，例如<?xml ... ?>
    XMLPIElement,            ///操作处理元素，例如<?mypi ... ?>
    XMLDTDElement            ///说明文档类型的元素，例如<!DOCTYPE ...>
}    XMLElementType;

/////////////////////////////////////////////////////////////////////////
///CXMLElement是一个XML元素类型，该类型对应于一个XML文档中的开始元素、结束
///元素、单独元素、文字元素、注释元素等，是XML文件中的一段连续文字。该类型
///仅仅用于对XML文件进行读取时，作为中间元素。读入后的操作应当使用CXMLNode
///@author    
///@version    2.0,20051011
/////////////////////////////////////////////////////////////////////////
class CXMLElement//: public CBaseObject
{
public:
    ///构造函数，创建一个空的XML元素
    CXMLElement(void);

    ///析构函数，不释放属性列表空间    
    ~CXMLElement(void);

    ///删除属性列表空间
    void removePropertyList(void);
    
    virtual int isA(const char *objectType);
    virtual const char *getType(void);
    //virtual void output(Logger *pLogger,int indent=0);

    ///获取元素类型
    ///@return    元素类型
    XMLElementType getElementType(void);
    
    ///设置元素类型
    ///@param    type    元素类型
    void setElementType(XMLElementType type);
    
    ///获取元素名称，对于文字，就是文字正文，对于注释，就是注释正文
    ///@return    元素名称
    const char *getName(void);
    
    ///设置元素名称
    ///@param    name    元素名称
    void setName(const char *name);
    
    ///获得属性列表
    ///@return    属性列表
    CXMLPropertyList *getPropertyList(void);
    
    ///设置属性列表
    ///@param    propertyList    属性列表
    void setPropertyList(CXMLPropertyList *propertyList);
    
    ///增加属性
    ///@param    name    属性名称
    ///@param    value    属性值
    ///@return    1表示成功，0表示失败
    int addProperty(const char *name, const char *value);
    
    ///删除属性，如果有多个属性都是这个名称，则删除第一个
    ///@param    name    属性名称
    ///@return    1表示成功，0表示失败
    int removeProperty(const char *name);
    
    ///替换属性，如果有多个属性都是这个名称，则替换第一个
    ///@param    name    属性名称
    ///@param    newValue    新的属性值
    ///@return    1表示成功，0表示失败
    int replaceProperty(const char *name, const char *newValue);

    ///寻找属性，如果有多个属性都是这个名称，则只找第一个
    ///@param    name    属性名称
    ///@return    找到的属性值，没有该属性，则返回NULL，如果该属性没有值，则返回""
    const char *findProperty(const char *name);

    ///输出到文件中，根据该元素的规定方式写出
    ///@param    output    指定的文件
    ///@return    1表示成功，0表示失败
    int write(FILE *output);
    int write(char *output);
private:
    ///XML元素类型
    XMLElementType m_type;
    
    ///元素名称
    CStringPtr m_name;
    
    ///属性列表
    CXMLPropertyList *m_propertyList;
};

/////////////////////////////////////////////////////////////////////////
///XMLNodeType是一个XML树中的节点的类型枚举值
///@author    
///@version    2.0,20051011
/////////////////////////////////////////////////////////////////////////
typedef enum 
{
    XMLNotNode,            ///不是XML节点
    XMLTag,                ///XML的标记，从开始元素到结束元素，或者是单独元素
    XMLText,            ///XML文字，是一个或多个文字元素
    XMLComment            ///XML注释
}    XMLNodeType;

class CXMLNode;

/////////////////////////////////////////////////////////////////////////
///CXMLNodeList是一个XML节点的序列，用于存储一个XML节点下的所有子节点
///@author    
///@version    2.0,20051011
/////////////////////////////////////////////////////////////////////////
class CXMLNodeList: public vector<CXMLNode *>
{
public:
    ///构造函数，创建一个空的序列
    CXMLNodeList(void);
    
    ///析构函数
    virtual ~CXMLNodeList(void);

    virtual int isA(const char *objectType);
    virtual const char *getType(void);
    //virtual void output(Logger *pLogger,int indent=0);

    ///复制一份自己
    ///@return    复制产生的新节点
    CXMLNodeList *clone(void);

    ///寻找第一个符合条件的节点
    ///@param    type    要寻找的标记类型，XMLNotNode表示不确定标记类型
    ///@param    tagName    要寻找的标记名称，NULL表示不确定的标记名称
    ///@param    propertyName    要寻找的标记必须包含的属性名称，NULL表示不需要检查属性名
    ///@param    propertyValue    要寻找的标记必须包含的属性值，NULL表示不需要检查属性值，当propertyName为NULL时，此参数无意义
    ///@return    找到的节点，找不到则返回NULL
    CXMLNode *findNode(XMLNodeType type, const char *tagName, const char *propertyName=NULL, const char *propertyValue=NULL);
    
    ///寻找一个指定的节点
    ///@param    pNode    要寻找的节点
    ///@return    该节点的下标位置，找不到则返回-1
    int findNode(CXMLNode *pNode);

    ///输出到文件中，包括所有的节点
    ///@param    output    指定的文件
    ///@param    indent    缩进量
    ///@return    1表示成功，0表示失败
    int write(FILE *output, int indent=0);
    int write(char *output, int indent=0);
};

///XML操作函数类型定义，0表示需要结束操作，其他值表示继续操作
typedef int (* TXMLActionFunc)(CXMLNode * pNode, void * parameter);

///对XML的属性值操作函数的类型定义，0表示需要结束操作，其他值表示继续操作
typedef int (* TXMLPropertyActionFunc)(const char *value, void *parameter);

/////////////////////////////////////////////////////////////////////////
///CXMLAction是一个XML操作类，表示对一个XML节点或者字符串应当采用何种操作
///方式。这个操作类是供CXMLNode中的XAction专用的。使用者应当继承这个类，
///重写需要的方法，然后将其作为XAction的参数
///@author    
///@version    2.0,20051226
/////////////////////////////////////////////////////////////////////////
class CXMLAction
{
public:
    ///析构函数
    virtual ~CXMLAction(void)
    {
    }
    
    ///节点处理程序
    ///@param    pNode    要处理的节点
    ///@return    0表示需要结束操作，>0表示继续处理
    virtual int nodeHandler(CXMLNode *pNode);

    ///字符串处理程序
    ///@param    string    要处理的字符串
    ///@return    0表示需要结束操作，>0表示继续处理
    virtual int stringHandler(const char *string);
};

/*
下面是扩展XPATH的定义，扩展XPATH(以下称EXPATH)用于指定在XML文件中指定节点下的
满足特定要求的一组节点，或者一组字符串
其语法如下（其中的()[]*|+为元字符，不出现在最终的扩展XPATH上，但是\为转义字符，
后面的元字符应当理解为原来的字符）

EXPATH=(EXNODE)*[STRING_VALUE]
EXNODE=(/|/\(NUMBER\)|//)(TAG_NAME|\*)(,CONDITION)*
CONDITION=STRING_REF[=RESULT]
STRING_VALUE=/STRING_REF(,STRING_REF)*
STRING_REF=(@PROPERTY_NAME|?TEXT_TAG_NAME)

上面EXPATH的语义如下：
1 EXPATH
EXPATH说明了要指定的最终目标，它先使用若干个EXNODE，分层次地指定到某些节点上，
这些EXNODE是有次序的，总是先从指定节点，按照第一个EXNODE，找出若干节点，再分别
以这些节点为指定节点，按照第二个EXNODE，找到若干节点，以此类推。如果后面没有
PROPERTY_VALUE或者TEXT_TAG_VALUE，则EXPATH对应的是按照上述方式找到的一组节点。
如果后面有STRING_VALUE，则按照其要求，将找到一组字符串。
2 EXNODE
EXNODE说明了如何从指定节点按照一定条件，找到一组节点。其开头为/、/(NUMBER)或者//。
这里的/表示在指定节点的所有子节点中寻找，/(NUMBER)表示在NUMBER层子孙节点中寻找，
也就是说，/实际上就是/(1)的简化形式。//表示在指定节点的任意子孙节点中寻找。在这个
说明寻找范围的标识后面是对TagName的说明，可以直接指定一个TAG_NAME，也可以用*表示
任意的TagName。最后是若干个条件CONDITION（具体见后）。同时满足前两项要求，并满足
所有CONDITION的节点，就是满足条件的节点。注意，如果使用了//，某个满足条件的节点
的所有子节点将不包括在寻找范围中。
3 CONDITION
CONDITION说明了对节点的一个条件。说明条件的方式有两种，一种是属性条件，一种是文本
条件。属性条件的形式是@PROPERTY_NAME=RESULT，即指定的属性名对应的值是RESULT。也可以
不指定结果，那么只要指定属性存在即可。另一种是文本条件，形式为?TEXT_TAG_NAME=RESULT，
表示该节点中指定子节点的文本内容为RESULT，也可以不指定结果，那么只要有指定子节点即可。
这里，节点中的文本是指某个节点，其名称符合指定要求，其子节点只有一个，并且是一个
XMLTEXT，那么文本指该文本的内容
4 STRING_VALUE
STRING_VALUE用于指明某个节点中的指定字符串，它是由逗号分割的若干个STRING_REF组成，
每个STRING_REF用于指定某个字符串，如果前面一个没有定义，那么就寻找后面一个。如果
前面一个有定义了，后面的就不用了。如果从第一个找到最后一个，所有的STRING_REF都
没有定义，则报错
5 STRING_REF
STRING_REF用于指明某个节点的一个字符串。指定字符串的方式有两种，一种是@PROPERTY_NAME，
表示指定的属性值，另一种是@TEXT_TAG_NAEM，表示指定的子节点文本内容。

下面给出几个例子：
1 /user,@gendar=male/@name
  找到所有男用户的姓名
2 /user/address,?street/?street
  找到所有用户的地址中可能存在的街道名
3 /(2)search/parameters/parameter/@name
  向下两层中所有检索的参数名
4 // *,@type=boolean/@name,@originalName
  寻找所有子孙节点中类型为boolean的节点名称，如果名称没有定义，则使用origianlName属性
  （注意：上面//和*中应当是没有空格的）
*/

class CEXPathNode;

/////////////////////////////////////////////////////////////////////////
///CXMLNode是一个XML节点类，一个XML节点是指XML文件转化为树以后的中间某个
///节点，这个节点可能是标记，也可能是正文或者注释。每个节点都有自己的属性
///并且可能包含了一个序列的子节点
///@author    
///@version    2.0,20051011
/////////////////////////////////////////////////////////////////////////
class CXMLNode//: public CBaseObject
{
public:
    ///构造函数
    CXMLNode(void);
    
    ///析构函数，会递归析构所有的子节点，析构属性列表和属性，但是不会释放字符串空间
    virtual ~CXMLNode(void);
    
    virtual int isA(const char *objectType);
    virtual const char *getType(void);
    //virtual void output(Logger *pLogger,int indent=0);

    ///复制一份自己
    ///@param    includeSon    是否要同时复制子节点
    ///@return    复制产生的新节点
    CXMLNode *clone(bool includeSon=false);

    ///获取节点类型
    ///@return    节点类型
    XMLNodeType getNodeType(void);
    
    ///设置节点类型
    ///@param    type    节点类型
    void setNodeType(XMLNodeType type);
    
    ///获取节点名称，对于标记，是标记的名称，对于正文和注释，就是正文和注释的全文
    ///@return    节点名称
    const char *getName(void);
    
    ///设置节点名称
    ///@param    name    节点名称
    void setName(const char *name);
    
    ///获取属性列表
    ///@return    属性列表
    CXMLPropertyList *getPropertyList(void);
    
    ///设置属性列表
    ///@param    propertyList    属性列表
    void setPropertyList(CXMLPropertyList *propertyList);
    
    ///增加一个属性
    ///@param    name    名称
    ///@param    value    值
    ///@return    1表示成功，0表示失败
    int addProperty(const char *name, const char *value);
    
    ///增加一个属性
    ///@param    name    名称
    ///@param    value    值
    ///@return    1表示成功，0表示失败
    int addProperty(const char *name, int value);

    ///删除一个属性，如果有多个属性都是这个名称，则删除第一个
    ///@param    name    属性名称
    ///@return    1表示成功，0表示失败
    int removeProperty(const char *name);
    
    ///替换一个属性，如果有多个属性都是这个名称，则替换第一个
    ///@param    name    属性名称
    ///@param    newValue    属性值
    ///@return    1表示成功，0表示失败
    int replaceProperty(const char *name, const char *newValue);
    
    ///寻找一个属性，如果有多个属性都是这个名称，则只找第一个
    ///@param    name    属性名称
    ///@return    属性值，如果没有该属性，则为NULL，如果没有设置属性值，则为""
    const char *findProperty(const char *name);
    
    ///设置一个属性，如果原来有这个属性，则替换这个属性，否则就增加这个属性
    ///@param    name    属性名称
    ///@param    value    属性值
    ///@return    1表示成功，0表示失败
    int setProperty(const char *name, const char *value);
    
    ///获取子节点序列
    ///@return    子节点序列
    CXMLNodeList *getNodeList(void);
    
    ///设置子节点序列
    ///@param    nodeList    子节点序列
    void setNodeList(CXMLNodeList *nodeList);

    ///增加一个子节点
    ///@param    pNode    要增加的子节点
    void addNode(CXMLNode *pNode);
    CXMLNode *addNode(const char *pNodeName,const char *pNodeValue);
    
    ///获取父节点
    ///@return    得到的父节点，如果没有父节点，则返回NULL
    CXMLNode *getFather(void);
    
    ///获取子节点
    ///@param    offset    相对位置，>0表示向后，<0表示向前，0表示当前
    ///@param    from    开始寻找的位置，SEEK_END表示最后一个节点，SEEK_SET表示第一个节点
    ///@param    得到的子节点，如果没有这个子节点，则返回NULL
    CXMLNode *getSon(int offset, int from=SEEK_SET);
    
    ///获取兄弟节点
    ///@param    offset    相对位置，>0表示向后，<0表示向前，0表示当前
    ///@param    from    开始寻找的位置，SEEK_CUR表示当前节点，SEEK_END表示最后一个节点，SEEK_SET表示第一个节点
    ///@param    得到的兄弟节点，如果没有这个兄弟节点，则返回NULL
    CXMLNode *getBrother(int offset=1, int from=SEEK_CUR);
    
    ///寻找第一个符合条件的节点
    ///@param    type    要寻找的标记类型，XMLNotNode表示不确定标记类型
    ///@param    tagName    要寻找的标记名称，NULL表示不确定的标记名称
    ///@param    propertyName    要寻找的标记必须包含的属性名称，NULL表示不需要检查属性名
    ///@param    propertyValue    要寻找的标记必须包含的属性值，NULL表示不需要检查属性值，当propertyName为NULL时，此参数无意义
    ///@return    找到的节点，找不到则返回NULL
    CXMLNode *findNode(XMLNodeType type, const char *tagName, const char *propertyName=NULL, const char *propertyValue=NULL);

    ///确认一个节点是否符合条件
    ///@param    type    要确认的标记类型，XMLNotNode表示不确定标记类型
    ///@param    tagName    要确认的标记名称，NULL表示不确定的标记名称
    ///@param    propertyName    要确认的标记必须包含的属性名称，NULL表示不需要检查属性名
    ///@param    propertyValue    要确认的标记必须包含的属性值，NULL表示不需要检查属性值，当propertyName为NULL时，此参数无意义
    ///@return    1表示符合,0表示不符合
    int validNode(XMLNodeType type, const char *tagName, const char *propertyName=NULL, const char *propertyValue=NULL);
    
    ///输出到文件中，包括该节点以及所有的子节点
    ///@param    output    指定的文件
    ///@param    indent    缩进量
    ///@return    1表示成功，0表示失败
    int write(FILE *output, int indent=0);
    int write(char *output, int indent=0);

    ///对所有子节点进行一项指定的操作
    ///@param    pFunc    操作函数
    ///@param    parameter    操作函数的参数
    ///@param    type    要寻找的标记类型，XMLNotNode表示不确定标记类型
    ///@param    tagName    要寻找的标记名称，NULL表示不确定的标记名称
    ///@param    propertyName    要寻找的标记必须包含的属性名称，NULL表示不需要检查属性名
    ///@param    propertyValue    要寻找的标记必须包含的属性值，NULL表示不需要检查属性值，当propertyName为NULL时，此参数无意义
    ///@return    0表示操作被操作函数中断，其他值表示最后一次叫用操作函数时的返回值，-1表示没有可操作的对象
    int groupAction(TXMLActionFunc pFunc, void *parameter=NULL,
        XMLNodeType type=XMLTag, const char *tagName=NULL, 
        const char *propertyName=NULL, const char *propertyValue=NULL);

    ///对所有子节点的指定属性进行一项指定的操作
    ///@param    usePropertyName    指定的属性名
    ///@param    pFunc    操作函数
    ///@param    parameter    操作函数的参数
    ///@param    type    要寻找的标记类型，XMLNotNode表示不确定标记类型
    ///@param    tagName    要寻找的标记名称，NULL表示不确定的标记名称
    ///@param    propertyName    要寻找的标记必须包含的属性名称，NULL表示不需要检查属性名
    ///@param    propertyValue    要寻找的标记必须包含的属性值，NULL表示不需要检查属性值，当propertyName为NULL时，此参数无意义
    ///@return    0表示操作被操作函数中断，其他值表示最后一次叫用操作函数时的返回值，-1表示没有可操作的对象
    int groupAction(const char *usePropertyName, TXMLPropertyActionFunc pFunc, void *parameter=NULL,
        XMLNodeType type=XMLTag, const char *tagName=NULL, 
        const char *propertyName=NULL, const char *propertyValue=NULL);

    ///按照扩展XPATH，对所有节点或者其指定属性，叫用指定对象的特定方法
    ///@param    EXPath    指定的扩展XPATH，其语法定义见后面的详细定义
    ///@param    pAction    进行指定操作的对象
    ///@return    0表示操作被中断，其他值表示最后一次叫用操作对象时的返回值，-1表示没有可操作的对象,-2表示语法错误,-3表示索取的值没有定义
    int XAction(const char *EXPath, CXMLAction *pAction);
    
protected:
    ///XML节点类型
    XMLNodeType m_type;
    
    ///节点名称
    CStringPtr m_name;
    
    ///属性列表
    CXMLPropertyList *m_propertyList;
    
    ///子节点序列
    CXMLNodeList *m_nodeList;
    
    ///父节点
    CXMLNode *m_father;

    int EXNodeAction(CXMLNode *pEXNode, CXMLAction *pAction);


    ///对字符串进行操作，是XAction中使用的内部函数
    ///@param    pPath    指定的扩展XPath片段，没有EXNODE部分，头上的/已经去掉
    ///@param    pAction    进行指定操作的对象
    ///@return    返回结果按照XAction的定义
    int stringRefAction(const char *pPath,CXMLAction *pAction);
    
    ///对节点进行检查，是XAction中使用的内部函数
    ///@param    pPath    指定的扩展XPath片段，头上的范围指定已经去掉
    ///@param    pAction    进行指定操作的对象
    ///@param    depth    要检查的节点的深度，0表示自身，<0表示所有子孙节点
    ///@return    返回结果按照XAction的定义
    int nodeCheck(const char *pPath,CXMLAction *pAction, int depth);

    ///检查本节点是否满足要求，是XAction中使用的内部函数
    ///@param    pPath    指定的扩展XPath的片段，头上的范围指定已经去掉
    ///@param    pathTail    返回的指向处理完的XPath后一个字符，只有当返回值为1，且*pIsMatch=1时会返回
    ///@param    pIsMatch    返回是否符合，1表示符合，0表示不符合，只有当返回值为1时会返回
    ///@return    返回结果按照XAction的定义，1表示正常返回
    int matchNode(const char *pPath, const char **pathTail, int *pIsMatch);
    
    ///获取一个字符串引用的值，是XAction中使用的内部函数
    ///@param    pPath    指定的扩展XPath的片段，应当指向一个@或者?
    ///@param    pathTail    返回的指向处理完的XPath后一个字符，只有当返回值为1时会返回
    ///@param    pStringValue    返回的字符串值，NULL表示没有取到值，只有当返回值为1时会返回
    ///@return    返回结果按照XAction的定义(除了-3)，1表示正常返回
    int getStringValue(const char *pPath, const char **pathTail, const char **pStringValue);

    ///获取扩展XPATH中的一个token，是XAction中使用的内部函数
    ///@param    pPath    指定的扩展XPath片段，应当指向一个token的开始位置
    ///@param    pathTail    返回取完后的下一个字符
    ///@return    返回取到的token，会申请空间，叫用方负责释放
    const char *getToken(const char *pPath, const char **pathTail);
};

///XML群组操作，对一个节点的所有子节点进行一项指定的操作
///@param    pNode    指定的节点
///@param    pFunc    操作函数
///@param    parameter    操作函数的参数
///@param    type    要寻找的标记类型，XMLNotNode表示不确定标记类型
///@param    tagName    要寻找的标记名称，NULL表示不确定的标记名称
///@param    propertyName    要寻找的标记必须包含的属性名称，NULL表示不需要检查属性名
///@param    propertyValue    要寻找的标记必须包含的属性值，NULL表示不需要检查属性值，当propertyName为NULL时，此参数无意义
///@return    0表示操作被操作函数中断，其他值表示最后一次叫用操作函数时的返回值，-1表示没有可操作的对象
int XMLGroupAction(CXMLNode *pNode, TXMLActionFunc pFunc, void *parameter=NULL,
    XMLNodeType type=XMLTag, const char *tagName=NULL, 
    const char *propertyName=NULL, const char *propertyValue=NULL);

///XML群组操作，对一个节点序列进行一项指定的操作
///@param    pNodeList    指定的节点序列
///@param    pFunc    操作函数
///@param    parameter    操作函数的参数
///@param    type    要寻找的标记类型，XMLNotNode表示不确定标记类型
///@param    tagName    要寻找的标记名称，NULL表示不确定的标记名称
///@param    propertyName    要寻找的标记必须包含的属性名称，NULL表示不需要检查属性名
///@param    propertyValue    要寻找的标记必须包含的属性值，NULL表示不需要检查属性值，当propertyName为NULL时，此参数无意义
///@return    0表示操作被操作函数中断，其他值表示最后一次叫用操作函数时的返回值，-1表示没有可操作的对象
int XMLGroupAction(CXMLNodeList *pNodeList, TXMLActionFunc pFunc, void *parameter=NULL,
    XMLNodeType type=XMLTag, const char *tagName=NULL, 
    const char *propertyName=NULL, const char *propertyValue=NULL);

///XML群组操作，对一个节点的所有子节点的指定属性进行一项指定的操作
///@param    pNode    指定的节点
///@param    usePropertyName    指定的属性名
///@param    pFunc    操作函数
///@param    parameter    操作函数的参数
///@param    type    要寻找的标记类型，XMLNotNode表示不确定标记类型
///@param    tagName    要寻找的标记名称，NULL表示不确定的标记名称
///@param    propertyName    要寻找的标记必须包含的属性名称，NULL表示不需要检查属性名
///@param    propertyValue    要寻找的标记必须包含的属性值，NULL表示不需要检查属性值，当propertyName为NULL时，此参数无意义
///@return    0表示操作被操作函数中断，其他值表示最后一次叫用操作函数时的返回值，-1表示没有可操作的对象
int XMLGroupAction(CXMLNode *pNode, const char *usePropertyName, TXMLPropertyActionFunc pFunc, void *parameter=NULL,
    XMLNodeType type=XMLTag, const char *tagName=NULL, 
    const char *propertyName=NULL, const char *propertyValue=NULL);

///XML群组操作，对一个节点序列的指定属性进行一项指定的操作
///@param    pNodeList    指定的节点序列
///@param    usePropertyName    指定的属性名
///@param    pFunc    操作函数
///@param    parameter    操作函数的参数
///@param    type    要寻找的标记类型，XMLNotNode表示不确定标记类型
///@param    tagName    要寻找的标记名称，NULL表示不确定的标记名称
///@param    propertyName    要寻找的标记必须包含的属性名称，NULL表示不需要检查属性名
///@param    propertyValue    要寻找的标记必须包含的属性值，NULL表示不需要检查属性值，当propertyName为NULL时，此参数无意义
///@return    0表示操作被操作函数中断，其他值表示最后一次叫用操作函数时的返回值，-1表示没有可操作的对象
int XMLGroupAction(CXMLNodeList *pNodeList, const char *usePropertyName, TXMLPropertyActionFunc pFunc, void *parameter=NULL,
    XMLNodeType type=XMLTag, const char *tagName=NULL, 
    const char *propertyName=NULL, const char *propertyValue=NULL);

/*
prepareEXPath是将上面所述的EXPath转化为一个XML节点。在进行这项转化时，所有的
EXPath中的信息将变为CXMLNode中的信息。转化后的结果相当于按照下面DTD的形式：
<!ELEMENT EXPATH (EXNODE*,STRING_VALUE?)>
<!ELEMENT EXNODE (CONDITION*)>
<!ATTLIST EXNODE
    DEPTH CDATA #REQUIRED
    TAG_NAME CDATA #REQUIRED
>
<!ELEMENT CONDITION (STRING_REF)>
<!ATTLIST CONDITION
    RESULT CDATA #IMPLIED
>
<!ELEMENT STRING_REF EMPTY>
<!ATTLIST STRING_REF
    STRING_REF_TYPE CDATA #REQUIRED
    NAME CDATA #REQUIRED
>
<!ELEMENT STRING_VALUE (STRING_REF*)>    
*/
/////////////////////////////////////////////////////////////////////////
///CEXPathNode是一个扩展XPATH的准备类，用于将一个扩展XPATH转化为一个XML
///节点，转化的方式见上
///@author    
///@version    1.0,20060101
/////////////////////////////////////////////////////////////////////////
class CEXPathNode: public CXMLNode
{
public:
    ///构造函数
    CEXPathNode(void);
    
    ///准备一个扩展XPATH
    ///@param    EXPath    要准备的扩展XPATH
    ///@return    1表示成功，-2表示语法错误
    int prepare(const char *EXPath);
    
    ///判断是否已经完成了准备
    ///@返回    1表示已经准备，0表示没有准备
    int isPrepared(void);
private:
    ///构建一个EXNODE，也可能得到一个STRING_VALUE
    ///@return    得到的节点，返回NULL表示失败
    CXMLNode *makeEXNode(void);

    ///构建一个STRING_VALUE
    ///@return    得到的节点，返回NULL表示失败
    CXMLNode *makeStringValue(void);

    ///构建一个STRING_REF
    ///@return    得到的节点，返回NULL表示失败
    CXMLNode *getStringRef(void);

    ///获取一个token
    ///@return    得到的token，返回NULL表示该token是空的
    const char *getToken(void);

    const char *pPath;
    
    int prepared;
};

typedef void (*StartElement)(void *userData, const char *name, CXMLNode *node);

typedef void (*EndElement)(void *userData, const char *name);

typedef bool (*EndElement2)(void *userData, const char *name);

typedef struct _key {
    FILE *input;
    StartElement start;
    EndElement2 end;
    int ignoreText;
    int depth;
    bool shouldEnd;
} KEY;

/////////////////////////////////////////////////////////////////////////
///CXMLDoc是一个XML文档类，一个XML文档是指有单一根节点的Well Formed的XML
///文档。但是，对于开始的一些指示性标志，可以省略。
///@author    
///@version    2.0,20051011
/////////////////////////////////////////////////////////////////////////
class CXMLDoc//: public CBaseObject
{
public:
    ///构造函数，创建一个空的XML文档
    CXMLDoc(void);
    
    ///析构函数，释放本文档的所有节点
    ~CXMLDoc(void);
    
    virtual int isA(const char *objectType);
    virtual const char *getType(void);
    //virtual void output(Logger *pLogger,int indent=0);

    ///获取根节点
    ///@return    根节点
    CXMLNode *getRootNode(void);
    
    ///设置根节点
    ///@param    rootNode    根节点
    void setRootNode(CXMLNode *rootNode);
    
    ///获取编码方式
    ///@return    编码方式
    const char *getEncoding(void);
    
    ///设置编码方式
    ///@param    encoding    编码方式
    void setEncoding(const char *encoding);
    
    ///获取版本
    ///@return    版本
    const char *getVersion(void);
    
    ///设置版本
    ///@param    version    版本
    void setVersion(const char *version);
    
    ///获取根节点名称（在DOCTYPE中指定）
    ///@return    根节点名称
    const char *getRoot(void);
    
    ///设置根节点名称
    ///@param    root    根节点名称
    void setRoot(const char *root);
    
    ///获取DOCTYPE指定的DTD文件。目前只支持指向外部文件的情况，不能直接在XML中嵌入DTD定义
    ///@return    DTD文件名
    const char *getDoctype(void);
    
    ///设置DTD文件
    ///@param    doctype    DTD文件
    void setDoctype(const char *doctype);
    
    ///从文件中读取XML文件
    ///@param    filename    文件名
    ///@param    ignoreText    1表示忽略所有的正文，0表示不忽略正文
    ///@param    ignoreComment    表示是否忽略注释
    ///@exception    如果发生语法错误，则抛出CParseError
    void read(const char *filename,int ignoreText=0, bool ignoreComment=true);


    ///从文件中读取XML文件
    ///@param    input    文件
    ///@param    ignoreText    1表示忽略所有的正文，0表示不忽略正文
    ///@param    ignoreComment    表示是否忽略注释
    ///@exception    如果发生语法错误，则抛出CParseError
    void read(FILE *input,int ignoreText=0, bool ignoreComment=true);


    // Sax mode parse function, added by 章捷, 2006.06.12
    void setUserData(void *data);

    void readSax(FILE *input, StartElement start, EndElement end, int ignoreText=0);

    // 可重入
    void readSax2(KEY *pKey);

    ///从字符串中读取XML
    ///@param    string    要读入的字符串
    ///@param    ignoreText    1表示忽略所有的正文，0表示不忽略正文
    ///@exception    如果发生语法错误，则抛出CParseError
    void readString(const char *string, int ignoreText=0);

    ///将XML写入文件
    ///@param    filename    文件名
    ///@return    1表示成功，0表示失败
    int write(const char *filename);
    
    ///将XML写入文件
    ///@param    output    文件
    ///@return    1表示成功，0表示失败
    int write(FILE *output);

    ///将XML文件的头写入文件
    ///@param    output    文件
    ///@return    1表示成功，0表示失败
    int writeHead(FILE *output);

    ///将XML文件的头写入缓冲区
    ///@param    void
    ///@return    写入的长度
    int writeStringHead(void);


    int writeString(char *lpString);

private:
    ///根节点
    CXMLNode *m_rootNode;

    ///编码方式
    CStringPtr m_encoding;
    
    ///版本
    CStringPtr m_version;
    
    ///根节点名称
    CStringPtr m_root;

    ///DTD文件名
    CStringPtr m_doctype;

    ///读取XML文件时的当前节点
    CXMLElement m_curElement;
    
    ///当前的行号
    int m_lineNo;
    
    ///前一读入的字符，EOF表示无法记入前一读取字符
    int m_lastChar;

    ///在从字符串读入时使用的字符串缓冲
    const char *m_inputString;

    // Sax mode parse function, added by 章捷, 2006.06.12
    void *m_userdata;

    ///从XML文件中获取下一个节点
    ///@param    input    XML文件
    ///@param    ignoreText    1表示忽略所有的正文，0表示不忽略正文
    ///@param    ignoreComment    表示是否忽略注释
    ///@return    获取的节点，应当等于m_curElement，如果已经到文件结束，则返回NULL
    ///@exception    如果发生语法错误，则抛出CParseError
    CXMLElement *getNextElement(FILE *input, int ignoreText, bool ignoreComment=true);

    ///从XML文件中获取下一个节点
    ///@param    input    XML文件
    ///@return    获取的节点，应当等于m_curElement，如果已经到文件结束，则返回NULL
    ///@exception    如果发生语法错误，则抛出CParseError
    CXMLElement *getPureNextElement(FILE *input);

    ///获取下一个字符，将被记入m_lastChar，将自动计算m_lineNo;
    ///@param    input    输入文件
    ///@param    allowEOF    是否可以接受EOF，1表示可以接受，0表示不可以接受，如果收到EOF，则抛出异常
    ///@return    得到的字符
    ///@exception    如果allowEOF为0，读到一个EOF，则抛出CParseError
    int getNextChar(FILE *input,int allowEOF=0);
    
    ///获取下一个非空白字符，将被记入m_lastChar
    ///@param    input    输入文件
    ///@param    allowEOF    是否可以接受EOF，1表示可以接受，0表示不可以接受，如果收到EOF，则抛出异常
    ///@return    得到的字符
    ///@exception    如果allowEOF为0，读到一个EOF，则抛出CParseError
    int ignoreSpace(FILE *input,int allowEOF=0);

    ///退回一个已经读入的字符，如果不能退回，则放弃
    ///@param    input    输入文件
    void ungetLastChar(FILE *input);

    ///期望某个字符，如果不是，则抛出异常
    ///@param    ch    读到的字符
    ///@param    expectingChar    期望的字符
    ///@exception    如果不是期望的字符，则抛出CParseError
    void expectChar(int ch,int expectingChar);

    ///获取下一个名称
    ///@param    input    输入文件
    ///@param    allowValue    是否允许以值的形式给出的名称，指在引号中的情况
    ///@return    得到的名称，如果不是名称，则返回NULL
    ///@exception    发生语法错误，将抛出CParseError
    const char *getNextName(FILE *input,int allowValue=0);
    
    ///读入一个字符串，到指定的endString为止，endString将被读掉，但是不会在返回结果中，会自动做entity的转化
    ///@param    input    输入文件
    ///@param    endString    结束字符串
    ///@return    得到的字符串
    ///@exception    发生语法错误，将抛出CParseError
    const char *getString(FILE *input,const char *endString);

    ///读入一个属性列表，将会新申请一个属性列表
    ///@param    input    输入文件
    ///@return    得到的属性列表
    ///@exception    发生语法错误，将抛出CParseError
    CXMLPropertyList *getNextPropertyList(FILE *input);

    ///字符串空间
    char *m_stringBuffer;
    
    ///已经存储的字符串长度
    int m_curLength;
    
    ///已经申请的字符串空间长度
    int m_curBufferLength;

    ///创建一个新的空字符串
    void makeNewString(void);
    
    ///在现有的字符串中增加一个字符
    ///@param    ch    要增加的字符
    void appendChar(int ch);

    ///获取当前的字符串
    ///@return    当前的字符串
    const char *getString(void);
    
    ///获取当前的字符串根据entity转化后的字符串
    ///@return    返回的字符串
    const char *getTransferedString(void);
};

/////////////////////////////////////////////////////////////////////////
///CParseError是一个语法分析错误异常
///@author    
///@version    2.0,20051011
/////////////////////////////////////////////////////////////////////////
class CParseError//: public CBaseObject
{
public:
    ///构造函数
    ///@param    msg    错误信息，外部不必保护此内存空间
    ///@param    lineNo    错误的行号
    CParseError(const char *msg, int lineNo);
    
    ///析构函数
    ~CParseError(void);

    virtual int isA(const char *objectType);
    virtual const char *getType(void);
    //virtual void output(Logger *pLogger,int indent=0);

    ///获取错误信息
    ///@return    错误信息
    const char *getMsg(void);
    
    ///获取错误行号
    ///@return    错误行号
    int getLineNo(void);
private:
    ///存储错误信息
    const char *m_msg;
    
    ///存储错误行号
    int m_lineNo;
};



#endif
