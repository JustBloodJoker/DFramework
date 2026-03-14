#pragma once

#include "../../pch.h"
#include "ScriptToken.h"

class ScriptLexer {
public:
    ScriptLexer(std::string s);

    char Peek();
    char Advance();
    std::vector<ScriptToken> Tokenize();

private:
    std::string m_sSrc;
    size_t m_uPos = 0;
    int m_iLine = 1;

};