#ifndef LEXER_H
#define LEXER_H

#include <stddef.h>

typedef enum 
{
    TOK_KEYWORD,
    TOK_IDENTIFIER,
    TOK_INTEGER,
    TOK_FLOAT,
    TOK_CHARACTER,
    TOK_STRING,
    TOK_OPERATOR,
    TOK_SYMBOL
} TokenType;

typedef struct 
{
    char *lexeme;
    TokenType type;
    size_t line;
    size_t column;
} Token;

typedef struct 
{
    Token *items;
    size_t count;
    size_t capacity;
} TokenList;

typedef struct
{
    size_t line;
    size_t column;
    char message[160];
    char lexeme[128];
} LexError;

const char *token_type_name(TokenType type);
int tokenize_source(const char *source, TokenList *tokens, LexError *error);
void free_token_list(TokenList *tokens);

#endif
