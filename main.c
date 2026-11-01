/*
    Name         : P Nisha
    Date         : 01-11-2026
    Project Title: Lexical Analyzer

    Description:
    Developing a Lexical Analyzer in C to analyze source code and identify tokens such as keywords, identifiers, operators, constants, and special symbols. The project focuses on lexical analysis, token classification, and understanding the initial phase of compiler design through efficient string processing and pattern recognition.

    Technologies Used:
    Programming Language: C
    Compiler: GCC
    Development Environment: Linux terminal and Visual Studio Code
    Build Tool: GNU Make
*/

#include "lexer.h"
#include "frequency.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static char *read_file(const char *path)
{
    FILE *fp = fopen(path, "rb");
    char *buffer;
    long size;
    size_t read_count;

    if (!fp) 
    {
        fprintf(stderr, "Error: cannot open input file '%s'. Check the file name and path.\n", path);
        return NULL;
    }
    if (fseek(fp, 0, SEEK_END) != 0 || (size = ftell(fp)) < 0 || fseek(fp, 0, SEEK_SET) != 0) 
    {
        fprintf(stderr, "Error: could not determine the size of '%s'.\n", path);
        fclose(fp);
        return NULL;
    }

    buffer = (char *)malloc((size_t)size + 1);

    if (!buffer)
    {
        fprintf(stderr, "Error: insufficient memory to read '%s'.\n", path);
        fclose(fp);
        return NULL;
    }

    read_count = fread(buffer, 1, (size_t)size, fp);

    if (read_count != (size_t)size && ferror(fp))
    {
        fprintf(stderr, "Error: failed while reading '%s'.\n", path);
        free(buffer);
        fclose(fp);
        return NULL;
    }

    buffer[read_count] = '\0';

    fclose(fp);

    return buffer;
}

static void print_usage(const char *program)
{
    printf("Lexical Analyzer - C Programming Project\n");
    printf("Usage: %s <source-file.c>\n", program);
    printf("Example: %s sample.c\n", program);
}

int main(int argc, char **argv)
{
    char *source;
    TokenList tokens = {0};
    LexError error;

    if (argc != 2)
    {
        print_usage(argv[0]);
        if (argc == 1)
        {
            fprintf(stderr, "\nPrompt: Please provide a C source file to analyze.\n");
        }
        else
        {
            fprintf(stderr, "\nError: too many command-line arguments.\n");
        }

        return 2;

    }

    source = read_file(argv[1]);

    if (!source) 
        return 2;

    printf("Lexical Analyzer: analyzing file '%s'\n\n", argv[1]);

    if (!tokenize_source(source, &tokens, &error))
    {
        fprintf(stderr, "Error: %s '%s' at line %zu, column %zu\n",error.message, error.lexeme, error.line, error.column);
        fprintf(stderr, "\nCompilation terminated due to lexical error.\n");
        free(source);
        free_token_list(&tokens);
        return 1;
    }

    if (tokens.count == 0)
    {
        printf("No tokens found. The source file is empty or contains only whitespace/comments.\n");
    }
    else
    {
        for (size_t i = 0; i < tokens.count; ++i)
        {
            printf("Line %-4zu, Col %-4zu: %-18s Type = %s\n", tokens.items[i].line, tokens.items[i].column, tokens.items[i].lexeme, token_type_name(tokens.items[i].type));
        }
    }

    print_frequency_summary(&tokens);

    free(source);

    free_token_list(&tokens);
    
    return 0;
}
