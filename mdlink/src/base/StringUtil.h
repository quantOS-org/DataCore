
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

#ifndef BASE_STRINGUTIL_H
#define    BASE_STRINGUTIL_H

#include <string>
#include <vector>
#include <sstream>


std::vector<std::string> split(const std::string& s);

int split(std::vector<std::string>& vs, const std::string& s);

std::vector<std::string> split(const std::string& s, const std::string& delim);

int split(std::vector<std::string>& vs, const std::string& s,const std::string delim);

void tokenize(std::vector<std::string>& tokens, const std::string& str, 
    const std::string& delim = " ");

//void trim(std::string& src, const std::string& spaces =" \t\n\r" );
void extract_contract(std::string instcode, std::string& commodity, std::string& contract_no);

std::string trim(const std::string& src);

inline double toDouble(std::string str)
{
    double data;
    std::stringstream ss;
    ss << str;   
    ss >> data;
    return data;
}

inline int toInt(std::string str)
{
    int data;
    std::stringstream ss;
    ss << str;
    ss >> data;
    return data;
}

template <typename T>
inline std::string toString(T data)
{
    std::string str;
    std::stringstream ss;
    ss << data;
    ss >> str;
    return str;
}

inline void str2int(const std::string& str, int& num)
{
    std::stringstream stream(str);
    stream >> num;
}
#endif    

