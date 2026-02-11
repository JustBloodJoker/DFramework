#include "ScriptParser.h"
#include "ScriptNodes.h"

ScriptParser::ScriptParser(std::vector<ScriptToken> t) : m_vTokens(t) {}

ScriptToken ScriptParser::Peek() { return m_vTokens[m_uPos]; }

ScriptToken ScriptParser::Advance() { 
    return m_uPos < m_vTokens.size() ? m_vTokens[m_uPos++] : m_vTokens.back(); 
}

bool ScriptParser::Match(ScriptTokenType t) {
    if (Peek().Type == t) { 
        Advance(); 
        return true; 
    }
    return false;
}

std::shared_ptr<ASTNode> ScriptParser::ParseExpression() {
    return ParseEquality();
}

std::shared_ptr<ASTNode> ScriptParser::ParseEquality() {
    auto left = ParseTerm();
    while (Peek().Type == EQ || Peek().Type == GT || Peek().Type == LT) {
        auto op = Peek().Type;

        Advance();
        left = std::make_shared<BinaryOpNode>(left, op, ParseTerm());
    }
    return left;
}

std::shared_ptr<ASTNode> ScriptParser::ParseTerm() {
    auto left = ParseFactor();
    while (Peek().Type == PLUS || Peek().Type == MINUS) {
        auto op = Peek().Type;

        Advance();
        left = std::make_shared<BinaryOpNode>(left, op, ParseFactor());
    }
    return left;
}

std::shared_ptr<ASTNode> ScriptParser::ParseFactor() {
    auto left = ParsePrimary();
    while (Peek().Type == MUL || Peek().Type == DIV) {
        auto op = Peek().Type;

        Advance();
        left = std::make_shared<BinaryOpNode>(left, op, ParsePrimary());
    }
    return left;
}

std::shared_ptr<ASTNode> ScriptParser::ParsePrimary() {
    if (Match(LPAREN)) {
        auto expr = ParseExpression();
        Match(RPAREN);
        return expr;
    }
    if (Peek().Type == NUMBER) {
        float v = std::stof(Advance().Text);
        return std::make_shared<LiteralNode>(v);
    }
    if (Peek().Type == STRING) {
        return std::make_shared<LiteralNode>(Advance().Text);
    }
    if (Peek().Type == ID) {
        auto name = Advance().Text;
        std::shared_ptr<ASTNode> node;

        if (Match(LPAREN)) {
            auto callNode = std::make_shared<CallNode>(name);
            if (Peek().Type != RPAREN) {
                do {
                    callNode->Args.push_back(ParseExpression());
                } while ( Match(COMMA) );
            }
            Match(RPAREN);
            node = callNode;
        }
        else {
            node = std::make_shared<VarNode>(name);
        }

        while (Match(DOT)) {
            if (Peek().Type == ID) {
                std::string member = Advance().Text;
                if (Match(LPAREN)) {
                    auto methodNode = std::make_shared<MethodCallNode>(node, member);
                    if (Peek().Type != RPAREN) {
                        do {
                            methodNode->Args.push_back(ParseExpression());
                        } while (Match(COMMA));
                    }
                    Match(RPAREN);
                    node = methodNode;
                }
                else {
                    node = std::make_shared<MemberAccessNode>(node, member);
                }
            }
        }
        return node;
    }
    return std::make_shared<LiteralNode>(0);
}

std::shared_ptr<ASTNode> ScriptParser::ParseBlock() {
    Match(LBRACE);
    auto block = std::make_shared<BlockNode>();
    while (Peek().Type != RBRACE && Peek().Type != END) {
        block->Statements.push_back(ParseStatement());
    }
    Match(RBRACE);
    return block;
}

std::shared_ptr<ASTNode> ScriptParser::ParseStatement() {
    if (Match(IF)) {
        auto cond = ParseExpression();
        auto then = ParseBlock();
        return std::make_shared<IfNode>(cond, then, nullptr);
    }
    if (Match(WHILE)) {
        auto cond = ParseExpression();
        auto body = ParseBlock();
        return std::make_shared<WhileNode>(cond, body);
    }
    if (Match(LOOP)) {
        auto cond = ParseExpression();
        auto body = ParseBlock();
        return std::make_shared<PredicateRegisterNode>(cond, body, true);
    }

    auto startPos = m_uPos;
    auto expr = ParseExpression();

    if (Match(LBRACE)) {
        m_uPos--;
        auto body = ParseBlock();
        return std::make_shared<PredicateRegisterNode>(expr, body);
    }

    if (Match(ASSIGN)) {
        auto rvalue = ParseExpression();
        Match(SEMICOLON);

        if (auto v = std::dynamic_pointer_cast<VarNode>(expr)) {
            return std::make_shared<AssignNode>(v->Name, rvalue);
        }
        if (auto m = std::dynamic_pointer_cast<MemberAccessNode>(expr)) {
            return std::make_shared<AssignNode>(m, rvalue);
        }
    }

    Match(SEMICOLON);
    return expr;
}

void ScriptParser::Flush() {
	m_uPos = 0;
}
