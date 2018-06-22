//
// Created by Silviu-Ioan Filip on 08/06/2018.
//

#ifndef EFRAC_TOKENUTILS_H
#define EFRAC_TOKENUTILS_H
#include <string>


enum TokenType
{
    OPERATOR,
    OPERAND,
    UNKNOWN,
    FUNCTION,
    PAREN
};

enum AssociativityType
{
    LEFT_ASSOC,
    RIGHT_ASSOC
};

// trim from the start
std::string &ltrim(std::string &s);

// trim from the end
std::string &rtrim(std::string &s);

// trim from both ends
std::string &trim(std::string &s);

#endif //EFRAC_TOKENUTILS_H
