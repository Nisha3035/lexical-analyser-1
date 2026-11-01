#include "frequency.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct
{
    const char *lexeme;
    size_t count;
    size_t first_seen;
} Frequency;

static int compare_frequency(const void *a, const void *b)
{
    const Frequency *x = (const Frequency *)a;
    const Frequency *y = (const Frequency *)b;
    if (x->count < y->count)
        return 1;
    if (x->count > y->count)
        return -1;
    /* Stable-looking tie break: preserve first occurrence order. */
    if (x->first_seen < y->first_seen)
        return -1;
    if (x->first_seen > y->first_seen)
        return 1;
    return 0;
}

void print_frequency_summary(const TokenList *tokens)
{
    Frequency *items;
    size_t count = 0;

    if (!tokens || tokens->count == 0)
    {
        printf("\n--- Frequency Summary (Sorted by Count) ---\n");
        printf("(No tokens found.)\n");
        return;
    }

    items = (Frequency *)calloc(tokens->count, sizeof(Frequency));
    if (!items)
    {
        fprintf(stderr, "Warning: unable to allocate memory for frequency summary.\n");
        return;
    }

    for (size_t i = 0; i < tokens->count; ++i)
    {
        size_t j;
        for (j = 0; j < count; ++j)
        {
            if (strcmp(items[j].lexeme, tokens->items[i].lexeme) == 0)
            {
                items[j].count++;
                break;
            }
        }
        if (j == count)
        {
            items[count].lexeme = tokens->items[i].lexeme;
            items[count].count = 1;
            items[count].first_seen = i;
            count++;
        }
    }

    qsort(items, count, sizeof(Frequency), compare_frequency);
    printf("\n--- Frequency Summary (Sorted by Count) ---\n");
    for (size_t i = 0; i < count; ++i)
        printf("%-24s : %zu\n", items[i].lexeme, items[i].count);

    free(items);
}
