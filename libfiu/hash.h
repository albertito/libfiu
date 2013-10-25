
#ifndef _HASH_H
#define _HASH_H

/* Generic hash table. See hash.c for more information. */

#include <sys/types.h>		/* for size_t */
#include <stdint.h>		/* for int64_t */

typedef struct hash hash_t;

hash_t *hash_create(void (*destructor)(void *));
void hash_free(hash_t *h);

void *hash_get(hash_t *h, const char *key);
bool hash_set(hash_t *h, const char *key, void *value);
bool hash_del(hash_t *h, const char *key);


/* Generic cache. */

typedef struct cache cache_t;

cache_t *cache_create();
bool cache_resize(struct cache *c, size_t new_size);
void cache_free(cache_t *c);

bool cache_get(cache_t *c, const char *key, void **value);
bool cache_set(cache_t *c, const char *key, void *value);
bool cache_del(cache_t *c, const char *key);
bool cache_invalidate(cache_t *c);

#endif

