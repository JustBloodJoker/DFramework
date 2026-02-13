#include "ScriptLexer.h"

ScriptLexer::ScriptLexer(std::string s) : m_sSrc(s) {}

char ScriptLexer::Peek() { return m_uPos < m_sSrc.size() ? m_sSrc[m_uPos] : 0; }

char ScriptLexer::Advance() { return m_uPos < m_sSrc.size() ? m_sSrc[m_uPos++] : 0; }

std::vector<ScriptToken> ScriptLexer::Tokenize() {
    std::vector<ScriptToken> tokens;

    while ( Peek() ) {
        auto c = Peek();
        if (isspace(c)) {
            if (c == '\n') m_iLine++;
            Advance();
        }
        else if (isdigit(c)) {
            
            std::string num;
            while ( isdigit(Peek()) || Peek() == '.' ) num += Advance();
            tokens.push_back({ NUMBER, num, m_iLine });

        }
        else if (isalpha(c) || c == '_') {

            std::string id;
            while (isalnum(Peek()) || Peek() == '_') id += Advance();

            if (id == "if") {
                tokens.push_back({ IF, id, m_iLine });
            }
            else if (id == "else") {
                tokens.push_back({ ELSE, id, m_iLine });
            }
            else if (id == "while") {
                tokens.push_back({ WHILE, id, m_iLine });
            }
            else if (id == "loop") {
                tokens.push_back({ LOOP, id, m_iLine });
            }
            else if (id == "function") {
                tokens.push_back({ FUNCTION, id, m_iLine });
            }
            else if (id == "return") {
                tokens.push_back({ RETURN, id, m_iLine });
            }
            else if (id == "void") {
                tokens.push_back({ VOID, id, m_iLine });
            }
            else {
                tokens.push_back({ ID, id, m_iLine });
            }
        }
        else if (c == '"') {
            Advance();
            
            std::string str;
            while (Peek() && Peek() != '"') str += Advance();

            Advance();
            
            tokens.push_back({ STRING, str, m_iLine });
        }
        else {
            Advance();
            
            switch (c) {
            case '+': tokens.push_back({ PLUS, "+", m_iLine }); break;
            case '-': tokens.push_back({ MINUS, "-", m_iLine }); break;
            case '*': tokens.push_back({ MUL, "*", m_iLine }); break;
            case '/': tokens.push_back({ DIV, "/", m_iLine }); break;
            case '=':
                if (Peek() == '=') { Advance(); tokens.push_back({ EQ, "==", m_iLine }); }
                else tokens.push_back({ ASSIGN, "=", m_iLine });
                break;
            case '>': 
                if (Peek() == '=') { Advance(); tokens.push_back({ GE, ">=", m_iLine }); }
                else tokens.push_back({ GT, ">", m_iLine }); 
                break;
            case '<': 
                if (Peek() == '=') { Advance(); tokens.push_back({ LE, "<=", m_iLine }); }
                else tokens.push_back({ LT, "<", m_iLine }); 
                break;
            case '{': tokens.push_back({ LBRACE, "{", m_iLine }); break;
            case '}': tokens.push_back({ RBRACE, "}", m_iLine }); break;
            case '(': tokens.push_back({ LPAREN, "(", m_iLine }); break;
            case ')': tokens.push_back({ RPAREN, ")", m_iLine }); break;
            case '.': tokens.push_back({ DOT, ".", m_iLine }); break;
            case ';': tokens.push_back({ SEMICOLON, ";", m_iLine }); break;
            case ',': tokens.push_back({ COMMA, ",", m_iLine }); break;
            case '#':
                while (Peek() && Peek() != '\n') Advance();
                break;
            }
        }
    }

    tokens.push_back({ END, "", m_iLine });
    return tokens;
}