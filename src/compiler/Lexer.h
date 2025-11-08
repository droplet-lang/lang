/*
 * ============================================================
 *  Droplet 
 * ============================================================
 *  Copyright (c) 2025 Droplet Contributors
 *  All rights reserved.
 *
 *  Licensed under the MIT License.
 *  See LICENSE file in the project root for full license.
 *
 *  File: Lexer
 *  Created: 11/8/2025
 * ============================================================
 */
#ifndef DROPLET_LEXER_H
#define DROPLET_LEXER_H

#include <string>
#include <vector>
#include <unordered_map>

enum class TokenType {
    // Special
    END_OF_FILE,
    UNKNOWN,

    // Keywords
    KW_NEW,
    KW_DROP,
    KW_IF,
    KW_ELSE,
    KW_WHILE,
    KW_FOR,
    KW_RETURN,

    // Identifiers and literals
    IDENTIFIER,
    INT_LITERAL,
    DOUBLE_LITERAL,
    STRING_LITERAL,
    BOOL_LITERAL,
    NIL_LITERAL,

    // Operators
    PLUS,
    MINUS,
    MUL,
    DIV,
    MOD,
    ASSIGN,
    EQ,
    NEQ,
    LT,
    GT,
    LTE,
    GTE,

    // Punctuation
    LPAREN,
    RPAREN,
    LBRACE,
    RBRACE,
    LBRACKET,
    RBRACKET,
    SEMICOLON,
    COMMA
};

struct Token {
    TokenType type;
    std::string lexeme;
    int line;
    int column;

    Token(TokenType t, std::string l, int ln, int col)
        : type(t), lexeme(std::move(l)), line(ln), column(col) {}
};

class Lexer {
public:
    explicit Lexer(const std::string& source);

    std::vector<Token> tokenize();

private:
    const std::string& src;
    size_t pos = 0;
    int line = 1;
    int column = 1;

    [[nodiscard]] char peek() const;
    char advance();
    [[nodiscard]] bool isAtEnd() const;

    void skipWhitespaceAndComments();
    Token makeToken(TokenType type, const std::string& lexeme) const;
    Token scanToken();

    Token identifierOrKeyword();
    Token number();
    Token string();

    static const std::unordered_map<std::string, TokenType> keywords;
};

#endif //DROPLET_LEXER_H