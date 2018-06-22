//
// Created by Silviu-Ioan Filip on 08/06/2018.
//

#ifndef EFRAC_TOKENIZER_H
#define EFRAC_TOKENIZER_H
#include <string>
#include <vector>


class tokenizer {
public:
    tokenizer();
    explicit tokenizer(std::string mData);
    ~tokenizer();
    std::vector<std::string>& getTokens();
    void setData(std::string newData);

private:
    void tokenize();

    std::string data;
    std::vector<std::string> tokens;
};


#endif //EFRACLIB_BUILD_TOKENIZER_H
