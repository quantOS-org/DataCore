
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

#include <string>
#include <vector>
#include <iostream>
#include <utility>
#include "StringUtil.h"

using namespace std;


vector<string> split(const string& s) {
  vector<string> vs;
  split(vs, s);
  return vs;
}

int split(vector<string>& vs, const string& s) {
  tokenize(vs, s, " \t\n\r");  
  return vs.size();
}


vector<string> split(const string& s, const string& delim) {
  vector<string> vs;  
  tokenize(vs, s, delim);
  return vs;
}

int split(vector<string>& vs,  const string& s, const string delim) {  
  tokenize(vs, s, delim);
  return vs.size();
}

void tokenize(vector<string>& tokens, const string& str,
    const string& delim) {    
  int pre;
  int pos = 0;
  
  do {
    pre = pos;
    while (pos < str.size() && delim.find(str[pos]) == string::npos) {
      ++pos;
    }
    tokens.push_back(str.substr(pre, pos - pre));
    ++pos;
  }while(pos < str.size());

  if(delim.find(str[pos - 1]) != string::npos){
    tokens.push_back("");
  }
}

string trim(const std::string& src) {
    auto p1 = src.begin();
    //while (p1 != src.end() && isspace(*p1)) p1++;
    while (p1 != src.end() && *p1<=' ') p1++;
    if (p1 == src.end()) return "";
    
    auto p2 = p1 + (src.end() -  p1) - 1;
    //while (isspace(*p2) && p2>p1) p2--;
    while (*p2<= ' ' && p2>p1) p2--;

    return string(p1, p2+1);
}

void extract_contract(std::string instcode, std::string& commodity, std::string& contract_no)
{
    commodity.clear();
    contract_no.clear();
    int i = 0;
    for (i = 0; i < instcode.size(); i++) {
        if ((instcode[i] >= 'a' && instcode[i] <= 'z') ||
            (instcode[i] >= 'A' && instcode[i] <= 'Z')) {            
            commodity.push_back(instcode[i]);
        }  
        else {
            break;
        }
    }
    for (; i < instcode.size(); i++) {
        if (instcode[i] >= '0' && instcode[i] <= '9') {
            contract_no.push_back(instcode[i]);
        }
        else {
            break;
        }
    }
}

//const string trim(const std::string& src, const std::string& spaces){
//    std::string::size_type i, j;
//
//    for (i = 0; i < src.size(); ++i){
//        if (spaces.find(src[i]) == std::string::npos) break;
//    }
//
//    for (j = src.size() - 1; j >= i; --j){
//        if (spaces.find(src[j]) == std::string::npos) break;
//    }
//
//    return src.substr(i, j - i + 1);
//}


