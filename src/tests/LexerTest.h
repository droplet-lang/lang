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
 *  File: LexerTest
 *  Created: 11/8/2025
 * ============================================================
 */
#ifndef DROPLET_LEXERTEST_H
#define DROPLET_LEXERTEST_H

#include "../compiler/Lexer.h"
#include <iostream>
#include <cassert>
#include <vector>

inline void TEST_IDENTIFIER_AND_KEYWORD() {
    std::string code = "new drop myVar if else while return true false nil";
    Lexer lexer(code);
    auto tokens = lexer.tokenize();

    std::vector<TokenType> expected = {
        TokenType::KW_NEW,
        TokenType::KW_DROP,
        TokenType::IDENTIFIER,
        TokenType::KW_IF,
        TokenType::KW_ELSE,
        TokenType::KW_WHILE,
        TokenType::KW_RETURN,
        TokenType::BOOL_LITERAL,
        TokenType::BOOL_LITERAL,
        TokenType::NIL_LITERAL,
        TokenType::END_OF_FILE
    };

    assert(tokens.size() == expected.size());
    for (size_t i = 0; i < expected.size(); ++i) {
        assert(tokens[i].type == expected[i]);
    }
    std::cout << "[PASS] TEST_IDENTIFIER_AND_KEYWORD\n";
}

inline void TEST_INTEGER_AND_DOUBLE_LITERAL() {
    std::string code = "123 456.789";
    Lexer lexer(code);
    auto tokens = lexer.tokenize();

    assert(tokens[0].type == TokenType::INT_LITERAL && tokens[0].lexeme == "123");
    assert(tokens[1].type == TokenType::DOUBLE_LITERAL && tokens[1].lexeme == "456.789");
    assert(tokens[2].type == TokenType::END_OF_FILE);
    std::cout << "[PASS] TEST_INTEGER_AND_DOUBLE_LITERAL\n";
}

inline void TEST_STRING_LITERAL() {
    std::string code = "\"hello world\" \"another string\"";
    Lexer lexer(code);
    auto tokens = lexer.tokenize();

    assert(tokens[0].type == TokenType::STRING_LITERAL && tokens[0].lexeme == "hello world");
    assert(tokens[1].type == TokenType::STRING_LITERAL && tokens[1].lexeme == "another string");
    assert(tokens[2].type == TokenType::END_OF_FILE);
    std::cout << "[PASS] TEST_STRING_LITERAL\n";
}

inline void TEST_OPERATORS_AND_PUNCTUATION() {
    std::string code = "+ - * / % = == != < <= > >= ( ) { } [ ] ; ,";
    Lexer lexer(code);
    auto tokens = lexer.tokenize();

    std::vector<TokenType> expected = {
        TokenType::PLUS, TokenType::MINUS, TokenType::MUL, TokenType::DIV, TokenType::MOD,
        TokenType::ASSIGN, TokenType::EQ, TokenType::NEQ, TokenType::LT, TokenType::LTE,
        TokenType::GT, TokenType::GTE, TokenType::LPAREN, TokenType::RPAREN,
        TokenType::LBRACE, TokenType::RBRACE, TokenType::LBRACKET, TokenType::RBRACKET,
        TokenType::SEMICOLON, TokenType::COMMA, TokenType::END_OF_FILE
    };

    assert(tokens.size() == expected.size());
    for (size_t i = 0; i < expected.size(); ++i) {
        assert(tokens[i].type == expected[i]);
    }
    std::cout << "[PASS] TEST_OPERATORS_AND_PUNCTUATION\n";
}

inline void TEST_COMMENTS_AND_WHITESPACE() {
    std::string code = R"(
        // This is a comment
        new /* block comment */ drop
    )";
    Lexer lexer(code);
    auto tokens = lexer.tokenize();

    assert(tokens[0].type == TokenType::KW_NEW);
    assert(tokens[1].type == TokenType::KW_DROP);
    assert(tokens[2].type == TokenType::END_OF_FILE);
    std::cout << "[PASS] TEST_COMMENTS_AND_WHITESPACE\n";
}

inline void TEST_MIXED_CODE() {
    std::string code = R"(
        new myVar = 42;
        if myVar >= 10 {
            drop myVar;
        }
    )";
    Lexer lexer(code);
    auto tokens = lexer.tokenize();

    // Simple checks for presence of expected token types
    bool has_new=false, has_identifier=false, has_assign=false, has_int=false,
         has_if=false, has_gte=false, has_lbrace=false, has_drop=false, has_semicolon=false, has_rbrace=false;

    for (auto& t : tokens) {
        if (t.type == TokenType::KW_NEW) has_new = true;
        if (t.type == TokenType::IDENTIFIER) has_identifier = true;
        if (t.type == TokenType::ASSIGN) has_assign = true;
        if (t.type == TokenType::INT_LITERAL) has_int = true;
        if (t.type == TokenType::KW_IF) has_if = true;
        if (t.type == TokenType::GTE) has_gte = true;
        if (t.type == TokenType::LBRACE) has_lbrace = true;
        if (t.type == TokenType::KW_DROP) has_drop = true;
        if (t.type == TokenType::SEMICOLON) has_semicolon = true;
        if (t.type == TokenType::RBRACE) has_rbrace = true;
    }

    assert(has_new && has_identifier && has_assign && has_int && has_if &&
           has_gte && has_lbrace && has_drop && has_semicolon && has_rbrace);
    std::cout << "[PASS] TEST_MIXED_CODE\n";
}

inline void ALL_TESTS_COMBINED() {
    TEST_IDENTIFIER_AND_KEYWORD();
    TEST_INTEGER_AND_DOUBLE_LITERAL();
    TEST_STRING_LITERAL();
    TEST_OPERATORS_AND_PUNCTUATION();
    TEST_COMMENTS_AND_WHITESPACE();
    TEST_MIXED_CODE();
    std::cout << "[PASS] ALL_TESTS_COMBINED\n";
}

#endif //DROPLET_LEXERTEST_H