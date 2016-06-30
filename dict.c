#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <stdio.h>

#include "dict.h"

struct elt {
    struct elt *next;
    char * key;
    int value;
};

struct dict {
    int size;           /* size of the pointer table */
    int n;              /* number of elements stored */
    struct elt **table;
};

#define INITIAL_SIZE (1024)
#define GROWTH_FACTOR (2)
#define MAX_LOAD_FACTOR (1)

/* dictionary initialization code used in both DictCreate and grow */
Dict
internalDictCreate(int size)
{
    Dict d;
    int i;

    d = malloc(sizeof(*d));

    assert(d != 0);

    d->size = size;
    d->n = 0;
    d->table = malloc(sizeof(struct elt *) * d->size);

    assert(d->table != 0);

    for(i = 0; i < d->size; i++) d->table[i] = 0;

    return d;
}

Dict
DictCreate(void)
{
    return internalDictCreate(INITIAL_SIZE);
}

void
DictDestroy(Dict d)
{
    int i;
    struct elt *e;
    struct elt *next;

    for(i = 0; i < d->size; i++) {
        for(e = d->table[i]; e != 0; e = next) {
            next = e->next;

            free(e->key);
            free(e);
        }
    }

    free(d->table);
    free(d);
}

#define MULTIPLIER (97)

static unsigned long
hash_function(const char *s)
{
    unsigned const char *us;
    unsigned long h;

    h = 0;

    for(us = (unsigned const char *) s; *us; us++) {
        h = h * MULTIPLIER + *us;
    }

    return h;
}

static void
grow(Dict d)
{
    Dict d2;            /* new dictionary we'll create */
    struct dict swap;   /* temporary structure for brain transplant */
    int i;
    struct elt *e;

    d2 = internalDictCreate(d->size * GROWTH_FACTOR);

    for(i = 0; i < d->size; i++) {
        for(e = d->table[i]; e != 0; e = e->next) {
            /* note: this recopies everything */
            /* a more efficient implementation would
             * patch out the strdups inside DictInsert
             * to avoid this problem */
            DictInsert(d2, e->key, e->value);
        }
    }

    /* the hideous part */
    /* We'll swap the guts of d and d2 */
    /* then call DictDestroy on d2 */
    swap = *d;
    *d = *d2;
    *d2 = swap;

    DictDestroy(d2);
}

/* insert a new key-value pair into an existing dictionary */
void
DictInsert(Dict d, const char *key, int value)
{
    struct elt *e;
    unsigned long h;

    if(key != NULL && value != 0) {
        // assert(value);

        e = malloc(sizeof(*e));

        assert(e);

        e->key = strdup(key);
        e->value = value;

        h = hash_function(key) % d->size;

        e->next = d->table[h];
        d->table[h] = e;

        d->n++;

        /* grow table if there is not enough room */
        if(d->n >= d->size * MAX_LOAD_FACTOR) {
            grow(d);
        }
    }
}

/* return the most recently inserted value associated with a key */
/* or 0 if no matching key is present */
int
DictSearch(Dict d, const char *key)
{
    struct elt *e;

    for(e = d->table[hash_function(key) % d->size]; e != 0; e = e->next) {
        if(!strcmp(e->key, key)) {
            /* got it */
            return e->value;
        }
    }

    return 0;
}

/* delete the most recently inserted record with the given key */
/* if there is no such record, has no effect */
void
DictDelete(Dict d, const char *key)
{
    struct elt **prev;          /* what to change when elt is deleted */
    struct elt *e;              /* what to delete */

    for(prev = &(d->table[hash_function(key) % d->size]); 
        *prev != 0; 
        prev = &((*prev)->next)) {
        if(!strcmp((*prev)->key, key)) {
            /* got it */
            e = *prev;
            *prev = e->next;

            free(e->key);
            free(e);

            return;
        }
    }
}

/* Print the dict contents to standard output */
char *
DictStringEncode(Dict d)
{
    struct elt *e;
    char * dict_str = NULL;
    int i, item_len;
    long int current_length = 0;
    char item_value[15];

    assert(d != 0);
    fprintf(stdout, "Encoding dictionary ...\n");

    for(i = 0; i < d->size; i++) {
        for(e = d->table[i]; e != 0; e = e->next) {
            // Convert the dict item value (an integer) to a string (for use in concatenation)
            sprintf(item_value, "%d", e->value);

            // Figure out how many characters this next item concatenation is going to use
            item_len = strlen(e->key) + strlen(item_value) + 2;

            // Allocate memory as needed
            dict_str = (char *) realloc( dict_str, current_length + item_len + 1);

            // Concatenate a represenation of the current dict item to the buffer
            current_length += sprintf(dict_str + current_length, "%s:%s,", e->key, item_value);    
        }
    }
    return dict_str;
}

/* Print the dict contents to standard output */
void 
DictPrint(Dict d)
{
    struct elt *e;
    int i;

    assert(d != 0);

    for(i = 0; i < d->size; i++) {
        for(e = d->table[i]; e != 0; e = e->next) {
            fprintf(stdout, "dict[%s] = %d\n", e->key, e->value);
        }
    }
}
