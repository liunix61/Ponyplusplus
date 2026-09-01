#ifndef PONYPP_GTEST_HELPERS_H
#define PONYPP_GTEST_HELPERS_H

#include <cstring>
#include "ponypp/lexer.h"
#include "ponypp/parser.h"
#include "ponypp/util.h"

static inline ASTNode* parse_to_ast(const char* src) {
    Lexer* lex = lexer_new("test.pny", src, std::strlen(src));
    Token* tokens = nullptr;
    size_t count = 0;
    if (!lexer_lex_all(lex, &tokens, &count)) {
        lexer_free(lex);
        if (tokens) {
            for (size_t i = 0; i < count; i++) free(tokens[i].value);
            free(tokens);
        }
        return nullptr;
    }
    Parser* p = parser_new("test.pny", tokens, count);
    ASTNode* ast = parser_parse_program(p);
    parser_free(p);
    lexer_free(lex);
    if (tokens) {
        for (size_t i = 0; i < count; i++) free(tokens[i].value);
        free(tokens);
    }
    return ast;
}

static inline void parse_and_free(const char* src) {
    ASTNode* ast = parse_to_ast(src);
    ast_node_free(ast);
}

#endif
