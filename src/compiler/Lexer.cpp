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
#include "Lexer.h"

#include <cctype>
#include <stdexcept>

const std::unordered_map<std::string, TokenType> Lexer::keywords = {
    {"new", TokenType::KW_NEW},
    {"drop", TokenType::KW_DROP},
    {"if", TokenType::KW_IF},
    {"else", TokenType::KW_ELSE},
    {"while", TokenType::KW_WHILE},
    {"for", TokenType::KW_FOR},
    {"return", TokenType::KW_RETURN},
    {"true", TokenType::BOOL_LITERAL},
    {"false", TokenType::BOOL_LITERAL},
    {"nil", TokenType::NIL_LITERAL}
};

Lexer::Lexer(const std::string& source) : src(source) {}

std::vector<Token> Lexer::tokenize() {
    std::vector<Token> tokens;
    while (!isAtEnd()) {
        skipWhitespaceAndComments();
        if (isAtEnd()) break;
        tokens.push_back(scanToken());
    }
    tokens.emplace_back(TokenType::END_OF_FILE, "", line, column);
    return tokens;
}

char Lexer::peek() const {
    if (isAtEnd()) return '\0';
    return src[pos];
}

char Lexer::advance() {
    if (isAtEnd()) return '\0';
    const char c = src[pos++];
    if (c == '\n') {
        line++;
        column = 1;
    } else {
        column++;
    }
    return c;
}

bool Lexer::isAtEnd() const {
    return pos >= src.size();
}

void Lexer::skipWhitespaceAndComments() {
    while (!isAtEnd()) {
        char c = peek();
        if (isspace(c)) {
            advance();
        } else if (c == '/' && pos+1 < src.size() && src[pos+1] == '/') {
            while (!isAtEnd() && peek() != '\n') advance();
        } else if (c == '/' && pos+1 < src.size() && src[pos+1] == '*') {
            advance(); advance();
            while (!isAtEnd() && !(peek() == '*' && pos+1 < src.size() && src[pos+1] == '/')) advance();
            if (!isAtEnd()) { advance(); advance(); }
        } else {
            break;
        }
    }
}

Token Lexer::makeToken(TokenType type, const std::string& lexeme) const {
    return {type, lexeme, line, column - static_cast<int>(lexeme.size())};
}

Token Lexer::scanToken() {
    char c = advance();
    switch (c) {
        case '+': return makeToken(TokenType::PLUS, "+");
        case '-': return makeToken(TokenType::MINUS, "-");
        case '*': return makeToken(TokenType::MUL, "*");
        case '/': return makeToken(TokenType::DIV, "/");
        case '%': return makeToken(TokenType::MOD, "%");
        case '=':
            if (peek() == '=') { advance(); return makeToken(TokenType::EQ, "=="); }
            return makeToken(TokenType::ASSIGN, "=");
        case '!':
            if (peek() == '=') { advance(); return makeToken(TokenType::NEQ, "!="); }
            return makeToken(TokenType::UNKNOWN, "!");
        case '<':
            if (peek() == '=') { advance(); return makeToken(TokenType::LTE, "<="); }
            return makeToken(TokenType::LT, "<");
        case '>':
            if (peek() == '=') { advance(); return makeToken(TokenType::GTE, ">="); }
            return makeToken(TokenType::GT, ">");
        case '(': return makeToken(TokenType::LPAREN, "(");
        case ')': return makeToken(TokenType::RPAREN, ")");
        case '{': return makeToken(TokenType::LBRACE, "{");
        case '}': return makeToken(TokenType::RBRACE, "}");
        case '[': return makeToken(TokenType::LBRACKET, "[");
        case ']': return makeToken(TokenType::RBRACKET, "]");
        case ';': return makeToken(TokenType::SEMICOLON, ";");
        case ',': return makeToken(TokenType::COMMA, ",");
        case '"': return string();
        default:
            if (isdigit(c)) return number();
            if (isalpha(c) || c == '_') return identifierOrKeyword();
            return makeToken(TokenType::UNKNOWN, std::string(1, c));
    }
}

Token Lexer::identifierOrKeyword() {
    const size_t start = pos - 1;
    while (!isAtEnd() && (isalnum(peek()) || peek() == '_')) advance();
    const std::string text = src.substr(start, pos - start);
    auto it = keywords.find(text);
    if (it != keywords.end()) return makeToken(it->second, text);
    return makeToken(TokenType::IDENTIFIER, text);
}

Token Lexer::number() {
    const size_t start = pos - 1;
    bool isDouble = false;
    while (!isAtEnd() && isdigit(peek())) advance();
    if (peek() == '.') {
        isDouble = true;
        advance();
        while (!isAtEnd() && isdigit(peek())) advance();
    }
    const std::string text = src.substr(start, pos - start);
    return makeToken(isDouble ? TokenType::DOUBLE_LITERAL : TokenType::INT_LITERAL, text);
}

Token Lexer::string() {
    const size_t start = pos;
    while (!isAtEnd() && peek() != '"') advance();
    const std::string text = src.substr(start, pos - start);
    if (!isAtEnd()) advance(); // consume closing "
    return makeToken(TokenType::STRING_LITERAL, text);
}

