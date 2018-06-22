//
// Created by Silviu-Ioan Filip on 08/06/2018.
//

#ifndef EFRAC_SHUNTINGYARD_H
#define EFRAC_SHUNTINGYARD_H
#include <string>
#include <vector>
#include <map>
#include <utility>
#include <functional>
#include <mpreal.h>
#include "tokenutils.h"

using namespace std;

class shuntingyard {
public:
    shuntingyard();
    mpfr::mpreal evaluate(std::vector<std::string> &tokens,
                          mpfr::mpreal x)const;

private:
    std::map<std::string, std::pair<int, int> > operatorMap;
    std::map<std::string, std::function<mpfr::mpreal(mpfr::mpreal)> > functionMap;
    std::map<std::string, std::function<mpfr::mpreal(mpfr::mpreal, mpfr::mpreal)> > operationMap;

    bool isOperator(std::string const &token)const;
    bool isFunction(std::string const &token)const;
    bool isOperand(std::string const &token)const;
    bool isAssociative(std::string const &token,
                       AssociativityType type)const;
    int cmpPrecedence(std::string const &token1,
                      std::string const &token2)const;
};


#endif //EFRAC_SHUNTINGYARD_H
