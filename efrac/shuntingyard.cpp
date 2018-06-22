//
// Created by Silviu-Ioan Filip on 08/06/2018.
//

#include "shuntingyard.h"
#include <stack>
#include <queue>
#include <iostream>

shuntingyard::shuntingyard()
{
    operatorMap["+"] = std::pair<int, AssociativityType>(0, LEFT_ASSOC);
    operatorMap["-"] = std::pair<int, AssociativityType>(0, LEFT_ASSOC);
    operatorMap["*"] = std::pair<int, AssociativityType>(4, LEFT_ASSOC);
    operatorMap["/"] = std::pair<int, AssociativityType>(4, LEFT_ASSOC);
    operatorMap["^"] = std::pair<int, AssociativityType>(6, RIGHT_ASSOC);

    functionMap["sin"] = [](mpfr::mpreal x) -> mpfr::mpreal { return mpfr::sin(x); };
    functionMap["cos"] = [](mpfr::mpreal x) -> mpfr::mpreal { return mpfr::cos(x); };
    functionMap["tan"] = [](mpfr::mpreal x) -> mpfr::mpreal { return mpfr::tan(x); };
    functionMap["asin"] = [](mpfr::mpreal x) -> mpfr::mpreal { return mpfr::asin(x); };
    functionMap["acos"] = [](mpfr::mpreal x) -> mpfr::mpreal { return mpfr::acos(x); };
    functionMap["atan"] = [](mpfr::mpreal x) -> mpfr::mpreal { return mpfr::atan(x); };
    functionMap["log"] = [](mpfr::mpreal x) -> mpfr::mpreal { return mpfr::log(x); };
    functionMap["log2"] = [](mpfr::mpreal x) -> mpfr::mpreal { return mpfr::log2(x); };
    functionMap["log10"] = [](mpfr::mpreal x) -> mpfr::mpreal { return mpfr::log10(x); };
    functionMap["exp"] = [](mpfr::mpreal x) -> mpfr::mpreal { return mpfr::exp(x); };
    functionMap["exp2"] = [](mpfr::mpreal x) -> mpfr::mpreal { return mpfr::exp2(x); };
    functionMap["exp10"] = [](mpfr::mpreal x) -> mpfr::mpreal { return mpfr::exp10(x); };
    functionMap["sqrt"] = [](mpfr::mpreal x) -> mpfr::mpreal { return mpfr::sqrt(x); };

    operationMap["+"] = [](mpfr::mpreal x, mpfr::mpreal y) -> mpfr::mpreal { return x + y; };
    operationMap["-"] = [](mpfr::mpreal x, mpfr::mpreal y) -> mpfr::mpreal { return x - y; };
    operationMap["*"] = [](mpfr::mpreal x, mpfr::mpreal y) -> mpfr::mpreal { return x * y; };
    operationMap["/"] = [](mpfr::mpreal x, mpfr::mpreal y) -> mpfr::mpreal { return x / y; };
    operationMap["^"] = [](mpfr::mpreal x, mpfr::mpreal y) -> mpfr::mpreal { return mpfr::pow(x, y); };
}

mpfr::mpreal shuntingyard::evaluate(std::vector<std::string> &tokens,
                                    mpfr::mpreal x) const
{

    std::stack<std::pair<std::string, int>> operatorStack;
    std::stack<mpfr::mpreal> operandStack;

    /*
    std::cout << "Shunting yard tokens:\n";
    for(auto &it : tokens)
        std::cout << it << std::endl;
    std::cout << std::endl;

    std::cout << "Token size:" << tokens.size() << std::endl;
    for(auto &it : tokens)
        std::cout << it << " ";
    std::cout << std::endl;
    */

    for (std::size_t i{0u}; i < tokens.size(); ++i)
    {
        tokens[i] = trim(tokens[i]);
        /*
        std::cout << "Token: " << tokens[i] << std::endl;
        std::stack<std::pair<std::string, int>> noperatorStack = operatorStack;
        std::stack<mpfr::mpreal> noperandStack = operandStack;
        std::cout << "Operator stack:\n";
        while(!noperatorStack.empty())
        {
            std::cout << noperatorStack.top().first << " ";
            noperatorStack.pop();
        }
        std::cout << std::endl;
        std::cout << "Operand stack:\n";
        while(!noperandStack.empty())
        {
            std::cout << noperandStack.top() << " ";
            noperandStack.pop();
        }
        std::cout << std::endl;
        */
        if (isOperator(tokens[i]))
        {
            if (operandStack.empty())
            {
                operatorStack.push(make_pair(tokens[i], 1)); // push a unary operator
            }
            else
            {
                if (i > 0 && (isOperator(tokens[i - 1]) || tokens[i - 1] == "("))
                {
                    // prefix unary operators
                    while (!operatorStack.empty() && isOperator(operatorStack.top().first)
                                                  && operatorStack.top().second == 1)
                    {
                        if (operatorStack.top().first == "+")
                            operatorStack.pop();
                        else if (operatorStack.top().first == "-")
                        {
                            operatorStack.pop();
                            mpfr::mpreal result = -operandStack.top();
                            operandStack.pop();
                            operandStack.push(result);
                        }
                        else
                        {
                            std::cerr << "Badly formatted input: processing error!";
                            exit(EXIT_FAILURE);
                        }
                    }
                    operatorStack.push(make_pair(tokens[i], 1));
                }
                else if (i > 0 && (isOperand(tokens[i - 1]) || tokens[i - 1] == ")"))
                {
                    // binary operators or postfix unary operators
                    while (!operatorStack.empty() && isOperator(operatorStack.top().first))
                    {
                        if ((isAssociative(tokens[i], LEFT_ASSOC) 
                                && cmpPrecedence(tokens[i], operatorStack.top().first) <= 0)
                                || (cmpPrecedence(tokens[i], operatorStack.top().first) < 0))
                        {
                            if (operatorStack.top().second == 2)
                            {
                                mpfr::mpreal op2 = operandStack.top();
                                operandStack.pop();
                                mpfr::mpreal op1 = operandStack.top();
                                operandStack.pop();
                                auto f = operationMap.find(operatorStack.top().first);
                                operandStack.push(f->second(op1, op2));
                                operatorStack.pop();
                            }
                            else
                            {
                                if (operatorStack.top().first == "+")
                                    operatorStack.pop();
                                else if (operatorStack.top().first == "-")
                                {
                                    operatorStack.pop();
                                    mpfr::mpreal result = -operandStack.top();
                                    operandStack.pop();
                                    operandStack.push(result);
                                }
                                else
                                {
                                    std::cerr << "Badly formatted input: processing error!";
                                    exit(EXIT_FAILURE);
                                }
                            }
                            continue;
                        }
                        break;
                    }
                    operatorStack.push(make_pair(tokens[i], 2));
                }
            }
        }
        else if (isFunction(tokens[i]) || tokens[i] == "(")
        {
            operatorStack.push(make_pair(tokens[i], 3));
        }
        else if (tokens[i] == ")")
        {
            while (operatorStack.top().first != "(")
            {
                if (isFunction(operatorStack.top().first))
                {
                    mpfr::mpreal op = operandStack.top();
                    operandStack.pop();
                    auto f = functionMap.find(operatorStack.top().first);
                    operandStack.push(f->second(op));
                }
                else if (operatorStack.top().second == 1)
                {
                    if (operatorStack.top().first == "+")
                    {
                    }
                    else if (operatorStack.top().first == "-")
                    {
                        mpfr::mpreal result = -operandStack.top();
                        operandStack.pop();
                        operandStack.push(result);
                    }
                    else
                    {
                        std::cerr << "Badly formatted input: processing error!";
                        exit(EXIT_FAILURE);
                    }
                }
                else
                {
                    mpfr::mpreal op2 = operandStack.top();
                    operandStack.pop();
                    mpfr::mpreal op1 = operandStack.top();
                    operandStack.pop();
                    auto f = operationMap.find(operatorStack.top().first);
                    operandStack.push(f->second(op1, op2));
                }
                operatorStack.pop();
            }
            operatorStack.pop();
            if (!operatorStack.empty() && isFunction(operatorStack.top().first))
            {
                mpfr::mpreal op = operandStack.top();
                operandStack.pop();
                auto f = functionMap.find(operatorStack.top().first);
                operandStack.push(f->second(op));
                operatorStack.pop();
            }
        }
        else
        {
            // numeric token
            if (tokens[i] == "pi")          // the constant Pi
                operandStack.push(mpfr::const_pi());
            else if (tokens[i] == "x")      // the unknown argument for the expression
                operandStack.push(x);
            else                            // assume a numeric constant otherwise,
                                            // parsable by the mprf/mpreal constructors
            {
                operandStack.push(mpfr::mpreal(tokens[i]));
            }
        }
    }

    while (!operatorStack.empty())
    {
        if (isFunction(operatorStack.top().first))
        {
            mpfr::mpreal op = operandStack.top();
            operandStack.pop();
            auto f = functionMap.find(operatorStack.top().first);
            operandStack.push(f->second(op));
        }
        else if (operatorStack.top().second == 1)
        {
            if (operatorStack.top().first == "+")
            {
            }
            else if (operatorStack.top().first == "-")
            {
                mpfr::mpreal result = -operandStack.top();
                operandStack.pop();
                operandStack.push(result);
            }
            else
            {
                std::cerr << "Badly formatted input: processing error!";
                exit(EXIT_FAILURE);
            }
        }
        else
        {
            mpfr::mpreal op2 = operandStack.top();
            operandStack.pop();
            mpfr::mpreal op1 = operandStack.top();
            operandStack.pop();
            auto f = operationMap.find(operatorStack.top().first);
            operandStack.push(f->second(op1, op2));
        }
        operatorStack.pop();
    }

    if (operandStack.size() != 1)
    {
        std::cerr << "Badly formatted input: processing error!\n";
        exit(EXIT_FAILURE);
    }

    mpfr::mpreal result = operandStack.top();
    return result;
}

bool shuntingyard::isOperator(std::string const &token) const
{
    return token == "+" || token == "-" || token == "*" 
                        || token == "/" || token == "^";
}

bool shuntingyard::isFunction(std::string const &token) const
{
    return token == "sin" || token == "cos" || token == "tan"
            || token == "log" || token == "log2" || token == "log10"
            || token == "asin" || token == "atan" || token == "sqrt"
            || token == "exp" || token == "exp2" || token == "exp10"
            || token == "acos";
}

bool shuntingyard::isOperand(std::string const &token) const
{
    return !isOperator(token) && !isFunction(token)
                              && token != "(" && token != ")";
}

bool shuntingyard::isAssociative(std::string const &token,
                                    AssociativityType type) const
{
    std::pair<int, int> p = operatorMap.find(token)->second;
    if (p.second == type)
    {
        return true;
    }
    return false;
}

int shuntingyard::cmpPrecedence(std::string const &token1,
                                std::string const &token2) const
{
    std::pair<int, int> p1 = operatorMap.find(token1)->second;
    std::pair<int, int> p2 = operatorMap.find(token2)->second;

    return p1.first - p2.first;
}
