#pragma once

#undef VOID

enum ScriptTokenType {
    ID,
    NUMBER,
    STRING,
    PLUS,
    MINUS,
    MUL,
    DIV,
    ASSIGN,
    EQ,
    GT,
    LT,
    LE,
    GE,
    LBRACE,
    RBRACE,
    LPAREN,
    RPAREN,
    COMMA,
    IF,
    ELSE,
    WHILE,
    LOOP,
    FUNCTION,
    RETURN,
    VOID,
    DOT,
    SEMICOLON,
    END
};