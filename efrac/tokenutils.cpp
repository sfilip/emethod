//
// Created by Silviu-Ioan Filip on 08/06/2018.
//

#include "tokenutils.h"
#include <utility>
#include <algorithm>
#include <functional>
#include <locale>
#include <cctype>

std::string &ltrim(std::string &s) {
    s.erase(s.begin(),
            std::find_if(s.begin(), s.end(),
                         std::not1(std::ptr_fun<int, int>(std::isspace))));
    return s;
}

std::string &rtrim(std::string &s) {
    s.erase(std::find_if(s.rbegin(),
                         s.rend(),
                         std::not1(std::ptr_fun<int, int>(std::isspace))).base(),
            s.end());
    return s;
}


std::string &trim(std::string &s) {
    return ltrim(rtrim(s));
}
