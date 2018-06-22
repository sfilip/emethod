//
// Created by Silviu-Ioan Filip on 08/06/2018.
//

#include "tokenizer.h"
#include <regex>

tokenizer::tokenizer() : data("") {}

tokenizer::tokenizer(std::string mData)
        : data(mData)
{
    tokenize();
}

tokenizer::~tokenizer() {}

void tokenizer::setData(std::string newData)
{
    data = newData;
    tokenize();
}

std::vector<std::string>& tokenizer::getTokens()
{
    return tokens;
}

void tokenizer::tokenize()
{
    tokens.clear();
    // regular expression that matches all the possible tokens using the default ECMAScript grammar
    std::regex tokenizer("[ \t\n]*(asin|acos|atan|sin|cos|tan|exp2|exp10|exp|log2|log10|log|sqrt|pi|[[:d:]]+[.][[:d:]][e][-][[:d:]]+|[[:d:]]+[.][[:d:]][e][[:d:]]+|[[:d:]]+[.][[:d:]]+|[[:d:]]+|[[:d:]]+e[-][[:d:]]+|[[:d:]]+e[[:d:]]+|[^]|[*]|[/]|[+]|[-]|[(]|[)]|x)[ \t\n]*",
                         std::regex_constants::icase);

    std::sregex_token_iterator pos(data.cbegin(), data.cend(),
                                   tokenizer,       // token separator
                                   0);              // match the whole token

    // as tokens are discovered, add them to the global list of tokens
    std::sregex_token_iterator end;
    for ( ;pos != end; ++pos )
        tokens.push_back(pos->str());
}
