#include "lexer.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const char *keywords[] = {
    "auto", "break", "case", "char", "const", "continue", "default",
    "do", "double", "else", "enum", "extern", "float", "for", "goto",
    "if", "inline", "int", "long", "register", "restrict", "return",
    "short", "signed", "sizeof", "static", "struct", "switch", "typedef",
    "union", "unsigned", "void", "volatile", "while",
    "_Alignas", "_Alignof", "_Atomic", "_Bool", "_Complex", "_Generic",
    "_Imaginary", "_Noreturn", "_Static_assert", "_Thread_local"};

static int is_keyword(const char *s)
{
    size_t i;
    for (i = 0; i < sizeof(keywords) / sizeof(keywords[0]); ++i)
        if (strcmp(s, keywords[i]) == 0)
            return 1;
    return 0;
}

const char *token_type_name(TokenType type)
{
    switch (type)
    {
    case TOK_KEYWORD:
        return "Keyword";
    case TOK_IDENTIFIER:
        return "Identifier";
    case TOK_INTEGER:
        return "Integer Constant";
    case TOK_FLOAT:
        return "Float Constant";
    case TOK_CHARACTER:
        return "Character Constant";
    case TOK_STRING:
        return "String Literal";
    case TOK_OPERATOR:
        return "Operator";
    case TOK_SYMBOL:
        return "Symbol";
    default:
        return "Unknown";
    }
}

static char *copy_span(const char *s, size_t n)
{
    char *p = (char *)malloc(n + 1);
    if (!p)
        return NULL;
    memcpy(p, s, n);
    p[n] = '\0';
    return p;
}

static int push_token(TokenList *list, const char *start, size_t len, TokenType type, size_t line, size_t col)
{
    Token *next;
    if (list->count == list->capacity)
    {
        size_t newcap = list->capacity ? list->capacity * 2 : 32;
        next = (Token *)realloc(list->items, newcap * sizeof(Token));
        if (!next)
            return 0;
        list->items = next;
        list->capacity = newcap;
    }
    list->items[list->count].lexeme = copy_span(start, len);
    if (!list->items[list->count].lexeme)
        return 0;
    list->items[list->count].type = type;
    list->items[list->count].line = line;
    list->items[list->count].column = col;
    list->count++;
    return 1;
}

static void set_error(LexError *e, size_t line, size_t col, const char *message, const char *start, size_t len)
{
    size_t n;
    if (!e)
        return;
    e->line = line;
    e->column = col;
    snprintf(e->message, sizeof(e->message), "%s", message);
    n = len < sizeof(e->lexeme) - 1 ? len : sizeof(e->lexeme) - 1;
    if (start && n)
        memcpy(e->lexeme, start, n);
    e->lexeme[n] = '\0';
}

static int is_ident_start(unsigned char c)
{
    return isalpha(c) || c == '_';
}
static int is_ident_part(unsigned char c)
{
    return isalnum(c) || c == '_';
}

static int is_hex_digit(char c)
{
    return isdigit((unsigned char)c) || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F');
}

static int is_exp_marker(char c)
{
    return c == 'e' || c == 'E';
}

static int is_hex_exp_marker(char c)
{
    return c == 'p' || c == 'P';
}

static int is_float_suffix(char c)
{
    return c == 'f' || c == 'F' || c == 'l' || c == 'L';
}

static int is_integer_suffix(char c)
{
    return c == 'u' || c == 'U' || c == 'l' || c == 'L';
}

/* Returns token length, or 0 for a malformed numeric literal. */
static size_t scan_number(const char *s, size_t *bad_len, const char **why)
{
    size_t i = 0, digits = 0;
    int is_hex = 0, is_float = 0;

    *bad_len = 1;
    *why = "Malformed numeric constant";

    if (s[0] == '.')
    {
        is_float = 1;
        i++;
        while (isdigit((unsigned char)s[i]))
        {
            i++;
            digits++;
        }
        if (!digits)
            return 0;
        if (is_exp_marker(s[i]))
        {
            size_t exp_start = i++;
            if (s[i] == '+' || s[i] == '-')
                i++;
            size_t exp_digits = 0;
            while (isdigit((unsigned char)s[i]))
            {
                i++;
                exp_digits++;
            }
            if (!exp_digits)
            {
                *bad_len = i > exp_start ? i : 1;
                *why = "Malformed exponent in floating constant";
                return 0;
            }
        }
        if (is_float_suffix(s[i]))
            i++;
        if (is_ident_part((unsigned char)s[i]) || s[i] == '.')
        {
            while (is_ident_part((unsigned char)s[i]) || s[i] == '.')
                i++;
            *bad_len = i;
            *why = "Invalid numeric constant";
            return 0;
        }
        return i;
    }

    if (s[0] == '0' && (s[1] == 'x' || s[1] == 'X'))
    {
        is_hex = 1;
        i = 2;
        while (is_hex_digit(s[i]))
        {
            i++;
            digits++;
        }
        if (s[i] == '.')
        {
            is_float = 1;
            i++;
            while (is_hex_digit(s[i]))
            {
                i++;
                digits++;
            }
        }
        if (is_hex_exp_marker(s[i]))
        {
            is_float = 1;
            i++;
            if (s[i] == '+' || s[i] == '-')
                i++;
            size_t exp_digits = 0;
            while (isdigit((unsigned char)s[i]))
            {
                i++;
                exp_digits++;
            }
            if (!exp_digits)
            {
                *bad_len = i ? i : 1;
                *why = "Malformed exponent in hexadecimal floating constant";
                return 0;
            }
        }
        if (!digits)
        {
            while (is_ident_part((unsigned char)s[i]) || s[i] == '.')
                i++;
            *bad_len = i ? i : 2;
            *why = "Invalid hexadecimal constant";
            return 0;
        }
        if (is_float)
        {
            if (!(s[i - 1] == 'p' || s[i - 1] == 'P') && !(strchr(s, 'p') || strchr(s, 'P')))
            {
                /* Hexadecimal floats with a dot require a binary exponent in C. */
                size_t j = 2;
                while (j < i && s[j] != 'p' && s[j] != 'P')
                    j++;
                if (j == i)
                {
                    *bad_len = i;
                    *why = "Hexadecimal floating constant requires a p exponent";
                    return 0;
                }
            }
            if (is_float_suffix(s[i]))
                i++;
        }
        else
        {
            while (is_integer_suffix(s[i]))
                i++;
        }
        if (is_ident_part((unsigned char)s[i]) || s[i] == '.')
        {
            while (is_ident_part((unsigned char)s[i]) || s[i] == '.')
                i++;
            *bad_len = i;
            *why = "Invalid numeric constant";
            return 0;
        }
        return i;
    }

    while (isdigit((unsigned char)s[i]))
    {
        i++;
        digits++;
    }
    if (s[i] == '.')
    {
        is_float = 1;
        i++;
        while (isdigit((unsigned char)s[i]))
        {
            i++;
            digits++;
        }
    }
    if (is_exp_marker(s[i]))
    {
        is_float = 1;
        size_t exp_start = i++;
        if (s[i] == '+' || s[i] == '-')
            i++;
        size_t exp_digits = 0;
        while (isdigit((unsigned char)s[i]))
        {
            i++;
            exp_digits++;
        }
        if (!exp_digits)
        {
            *bad_len = i > exp_start ? i : 1;
            *why = "Malformed exponent in floating constant";
            return 0;
        }
    }

    if (is_float)
    {
        if (is_float_suffix(s[i]))
            i++;
    }
    else
    {
        while (is_integer_suffix(s[i]))
            i++;
    }

    if (is_ident_part((unsigned char)s[i]) || s[i] == '.')
    {
        while (is_ident_part((unsigned char)s[i]) || s[i] == '.')
            i++;
        *bad_len = i;
        *why = "Invalid numeric constant";
        return 0;
    }

    /* Avoid unused variable warning and reject invalid octal digits. */
    if (!is_float && s[0] == '0' && i > 1 && s[1] >= '0' && s[1] <= '9')
    {
        size_t j = 1;
        while (j < i && isdigit((unsigned char)s[j]))
        {
            if (s[j] == '8' || s[j] == '9')
            {
                *bad_len = i;
                *why = "Invalid digit in octal integer constant";
                return 0;
            }
            j++;
        }
    }
    (void)is_hex;
    return i;
}

static int is_symbol(char c)
{
    return strchr("(){}[];,.:?", c) != NULL;
}

static const char *operators[] = {
    ">>=", "<<=", "...", "->", "++", "--", "==", "!=", "<=", ">=",
    "&&", "||", "+=", "-=", "*=", "/=", "%=", "&=", "|=", "^=",
    "<<", ">>", "##", "+", "-", "*", "/", "%", "=", "<", ">", "!",
    "&", "|", "^", "~", "#"};

int tokenize_source(const char *source, TokenList *tokens, LexError *error)
{
    size_t i = 0, line = 1, col = 1;
    if (!source || !tokens)
        return 0;
    memset(tokens, 0, sizeof(*tokens));
    if (error)
        memset(error, 0, sizeof(*error));

    while (source[i])
    {
        size_t start = i, start_line = line, start_col = col, len = 0;
        unsigned char c = (unsigned char)source[i];

        if (c == ' ' || c == '\t' || c == '\r' || c == '\n' || c == '\v' || c == '\f')
        {
            if (c == '\n')
            {
                line++;
                col = 1;
            }
            else
                col++;
            i++;
            continue;
        }

        if (source[i] == '/' && source[i + 1] == '/')
        {
            while (source[i] && source[i] != '\n')
            {
                i++;
                col++;
            }
            continue;
        }
        if (source[i] == '/' && source[i + 1] == '*')
        {
            i += 2;
            col += 2;
            while (source[i] && !(source[i] == '*' && source[i + 1] == '/'))
            {
                if (source[i] == '\n')
                {
                    line++;
                    col = 1;
                    i++;
                }
                else
                {
                    i++;
                    col++;
                }
            }
            if (!source[i])
            {
                set_error(error, start_line, start_col, "Unterminated block comment", source + start, i - start);
                return 0;
            }
            i += 2;
            col += 2;
            continue;
        }

        /* String and character literals, including common escapes. */
        if (source[i] == '"' || source[i] == '\'')
        {
            char quote = source[i];
            int closed = 0;
            i++;
            col++;
            while (source[i])
            {
                if (source[i] == '\n')
                {
                    set_error(error, start_line, start_col,
                              quote == '"' ? "Unterminated string literal" : "Unterminated character constant",
                              source + start, i - start);
                    return 0;
                }
                if (source[i] == '\\')
                {
                    if (source[i + 1] == '\0')
                        break;
                    i += 2;
                    col += 2;
                    continue;
                }
                if (source[i] == quote)
                {
                    i++;
                    col++;
                    closed = 1;
                    break;
                }
                i++;
                col++;
            }
            if (!closed)
            {
                set_error(error, start_line, start_col,
                          quote == '"' ? "Unterminated string literal" : "Unterminated character constant",
                          source + start, i - start);
                return 0;
            }
            len = i - start;
            if (quote == '\'' && len < 3)
            {
                set_error(error, start_line, start_col, "Empty character constant", source + start, len);
                return 0;
            }
            if (!push_token(tokens, source + start, len,
                            quote == '"' ? TOK_STRING : TOK_CHARACTER, start_line, start_col))
                goto memory_error;
            col = start_col + len;
            continue;
        }

        if (is_ident_start(c))
        {
            i++;
            col++;
            while (is_ident_part((unsigned char)source[i]))
            {
                i++;
                col++;
            }
            len = i - start;
            char *word = copy_span(source + start, len);
            if (!word)
                goto memory_error;
            TokenType type = is_keyword(word) ? TOK_KEYWORD : TOK_IDENTIFIER;
            free(word);
            if (!push_token(tokens, source + start, len, type, start_line, start_col))
                goto memory_error;
            continue;
        }

        if (isdigit(c) || (source[i] == '.' && isdigit((unsigned char)source[i + 1])))
        {
            const char *why = NULL;
            size_t bad_len = 1;
            len = scan_number(source + i, &bad_len, &why);
            if (!len)
            {
                set_error(error, start_line, start_col, why ? why : "Malformed numeric constant",
                          source + start, bad_len);
                return 0;
            }
            int is_float = 0;
            for (size_t k = 0; k < len; ++k)
                if (source[start + k] == '.' || source[start + k] == 'e' ||
                    source[start + k] == 'E' || source[start + k] == 'p' ||
                    source[start + k] == 'P' || source[start + k] == 'f' ||
                    source[start + k] == 'F')
                    is_float = 1;
            if (!push_token(tokens, source + start, len,
                            is_float ? TOK_FLOAT : TOK_INTEGER, start_line, start_col))
                goto memory_error;
            i += len;
            col += len;
            continue;
        }

        if (is_symbol((char)c))
        {
            if (!push_token(tokens, source + i, 1, TOK_SYMBOL, line, col))
                goto memory_error;
            i++;
            col++;
            continue;
        }

        int matched = 0;
        for (size_t op = 0; op < sizeof(operators) / sizeof(operators[0]); ++op)
        {
            size_t oplen = strlen(operators[op]);
            if (strncmp(source + i, operators[op], oplen) == 0)
            {
                if (!push_token(tokens, source + i, oplen, TOK_OPERATOR, line, col))
                    goto memory_error;
                i += oplen;
                col += oplen;
                matched = 1;
                break;
            }
        }
        if (matched)
            continue;

        {
            char bad[2] = {(char)c, '\0'};
            set_error(error, line, col, "Unrecognized character", bad, 1);
            return 0;
        }
    }
    return 1;

memory_error:
    set_error(error, line, col, "Out of memory while storing tokens", NULL, 0);
    return 0;
}

void free_token_list(TokenList *tokens)
{
    if (!tokens)
        return;
    for (size_t i = 0; i < tokens->count; ++i)
        free(tokens->items[i].lexeme);
    free(tokens->items);
    tokens->items = NULL;
    tokens->count = tokens->capacity = 0;
}
