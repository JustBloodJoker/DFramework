#pragma once

#include "../../pch.h"
#include "ScriptToken.h"
#include "ScriptTokenType.h"

struct ASTNode;

class ScriptParser {

public:
    ScriptParser(std::vector<ScriptToken> t);

    ScriptToken Peek();
    ScriptToken Advance();
    bool Match(ScriptTokenType t);

    std::shared_ptr<ASTNode> ParseExpression();
    std::shared_ptr<ASTNode> ParseEquality();
    std::shared_ptr<ASTNode> ParseTerm();
    std::shared_ptr<ASTNode> ParseFactor();
    std::shared_ptr<ASTNode> ParsePrimary();
    std::shared_ptr<ASTNode> ParseBlock();
    std::shared_ptr<ASTNode> ParseStatement();

    void Flush();

private:
    std::vector<ScriptToken> m_vTokens;
    size_t m_uPos = 0;

};
