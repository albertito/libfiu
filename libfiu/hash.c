
/*
 * Generic, simple hash table.
 *
 * Takes \0-terminated strings as keys, and void * as values.
 * It is tuned for a small number of elements (< 1000).
 *
 * It is NOT thread-safe.
 */

#include <sys/types.h>		/* for size_t */
#include <stdint.h>		/* for [u]int*_t */
#include <stdbool.h>		/* for bool */
#include <stdlib.h>		/* for malloc() */
#include <string.h>		/* for memcpy()/memcmp() */
#include <stdio.h>		/* snprintf() */
#include <pthread.h>		/* read-write locks */
#include "hash.h"

/* MurmurHash2, by Austin Appleby. The one we use.
 * It has been modify to fit into the coding style, to work on uint32_t
 * instead of ints, and the seed was fixed to a random number because it's not
 * an issue for us. The author placed it in the public domain, so it's ok to
 * use it here.
 * http://sites.google.com/site/murmurhash/ */
static uint32_t murmurhash2(const char *key, size_t len)
{
	const uint32_t m = 0x5bd1e995;
	const int r = 24;
	const uint32_t seed = 0x34a4b627;

	// Initialize the hash to a 'random' value
	uint32_t h = seed ^ len;

	// Mix 4 bytes at a time into the hash
	while (len >= 4) {
		uint32_t k = *(uint32_t *) key;

		k *= m;
		k ^= k >> r;
		k *= m;

		h *= m;
		h ^= k;

		key += 4;
		len -= 4;
	}

	// Handle the last few bytes of the input array
	switch (len) {
		case 3: h ^= key[2] << 16;
		case 2: h ^= key[1] << 8;
		case 1: h ^= key[0];
			h *= m;
	}

	// Do a few final mixes of the hash to ensure the last few
	// bytes are well-incorporated.
	h ^= h >> 13;
	h *= m;
	h ^= h >> 15;

	return h;
}

enum used_as {
	NEVER = 0,
	IN_USE = 1,
	REMOVED = 2,
};

struct entry {
	char *key;
	void *value;
	enum used_as in_use;
};

struct hash {
	struct entry *entries;
	size_t table_size;
	size_t nentries;
	size_t nremoved;
	void (*destructor)(void *);
};


/* Minimum table size. */
#define MIN_SIZE 10

/* Dumb destructor, used to simplify the code when no destructor is given. */
static void dumb_destructor(void *value)
{
	return;
}

struct hash *hash_create(void (*destructor)(void *))
{
	struct hash *h = malloc(sizeof(struct hash));
	if (h == NULL)
		return NULL;

	h->entries = malloc(sizeof(struct entry) * MIN_SIZE);
	if (h->entries == NULL) {
		free(h);
		return NULL;
	}

	memset(h->entries, 0, sizeof(struct entry) * MIN_SIZE);

	h->table_size = MIN_SIZE;
	h->nentries = 0;
	h->nremoved = 0;

	if (destructor == NULL)
		destructor = dumb_destructor;

	h->destructor = destructor;

	return h;
}

void hash_free(struct hash *h)
{
	size_t i;
	struct entry *entry;

	for (i = 0; i < h->table_size; i++) {
		entry = h->entries + i;
		if (entry->in_use == IN_USE) {
			h->destructor(entry->value);
			free(entry->key);
		}
	}

	free(h->entries);
	free(h);
}

void *hash_get(struct hash *h, const char *key)
{
	size_t pos;
	struct entry *entry;

	pos = murmurhash2(key, strlen(key)) % h->table_size;

	for (int i = 0; i < h->table_size; i++) {
		entry = h->entries + pos;
		if (entry->in_use == NEVER) {
			/* We got to a entry never used, no match. */
			return NULL;
		} else if (entry->in_use == IN_USE &&
				strcmp(key, entry->key) == 0) {
			/* The key matches. */
			return entry->value;
		}

		/* The key doesn't match this entry, continue with linear probing. */
		pos = (pos + 1) % h->table_size;
	}

	/* We went through the entire table and did not find the key.
	 * Note this is a pathological case that we don't expect would happen
	 * under normal operation, since we resize the table so there are
	 * always some never-used spaces. */
	return NULL;
}

/* Internal version of hash_set.
 * It uses the key as-is (it won't copy it), and it won't resize the array
 * either. */
static bool _hash_set(struct hash *h, char *key, void *value)
{
	size_t pos;
	struct entry *entry;

	pos = murmurhash2(key, strlen(key)) % h->table_size;

	for (int i = 0; i < h->table_size; i++) {
		entry = h->entries + pos;
		if (entry->in_use == NEVER) {
			/* Use only fresh entries, reusing removed ones
			 * introduces significant complications when combined
			 * with the linear probing. */
			entry->in_use = IN_USE;
			entry->key = key;
			entry->value = value;
			h->nentries++;
			return true;
		} else if (entry->in_use == IN_USE &&
				strcmp(key, entry->key) == 0) {
			/* The key matches, override the value. */
			h->destructor(entry->value);
			entry->value = value;
			return true;
		}

		/* The key doesn't match this entry, continue with linear probing. */
		pos = (pos + 1) % h->table_size;
	}

	/* We went through the entire table and did not find the key.
	 * Note this is a pathological case that we don't expect would happen
	 * under normal operation, since we resize the table so there are
	 * always some never-used spaces. */
	return false;
}

static bool resize_table(struct hash *h, size_t new_size)
{
	size_t i;
	struct entry *old_entries, *e;
	size_t old_size;

	/* Do not resize below minimum size; however, continue anyway to clear
	 * up removed entries. */
	if (new_size < MIN_SIZE) {
		new_size = MIN_SIZE;
	}

	old_entries = h->entries;
	old_size = h->table_size;

	h->entries = malloc(sizeof(struct entry) * new_size);
	if (h->entries == NULL)
		return false;

	memset(h->entries, 0, sizeof(struct entry) * new_size);
	h->table_size = new_size;
	h->nentries = 0;
	h->nremoved = 0;

	/* Insert the old entries into the new table. We use the internal
	 * version _hash_set() to avoid copying the keys again. */
	for (i = 0; i < old_size; i++) {
		e = old_entries + i;
		if (e->in_use == IN_USE)
			_hash_set(h, e->key, e->value);
	}

	free(old_entries);

	return true;
}

static bool auto_resize_table(hash_t *h) {
	/* Slots in the table is taken by valid and removed entries.
	 * Since we don't reuse removed slots, we need to also take them into
	 * consideration and shrink if we are accumulating too many.
	 *
	 * This function always resizes to <number of entries> * 2, which
	 * preserves the "at least 30% usable" property both when growing and
	 * shrinking, and makes calculations simpler.
	 */

	/* Maintain at least 30% usable entries. */
	float usable_ratio = (float) (h->table_size - h->nentries - h->nremoved)
		/ h->table_size;
	if (usable_ratio < 0.3) {
		return resize_table(h, h->nentries * 2);
	}

	/* If we have less than 30% used (by valid entries), compact. */
	float entries_ratio = (float) h->nentries / h->table_size;
	if (h->table_size > MIN_SIZE && entries_ratio < 0.3) {
		return resize_table(h, h->nentries * 2);
	}

	return true;
}


bool hash_set(struct hash *h, const char *key, void *value)
{
	bool r = _hash_set(h, strdup(key), value);

	if (!auto_resize_table(h)) {
		return false;
	}

	return r;
}

bool hash_del(struct hash *h, const char *key)
{
	size_t pos;
	struct entry *entry;
	bool found = false;

	pos = murmurhash2(key, strlen(key)) % h->table_size;

	for (int i = 0; i < h->table_size; i++) {
		entry = h->entries + pos;
		if (entry->in_use == NEVER) {
			/* We got to a never used key, not found. */
			return false;
		} else if (entry->in_use == IN_USE &&
				strcmp(key, entry->key) == 0) {
			/* The key matches, remove it. */
			found = true;
			break;
		}

		/* The key doesn't match this entry, continue with linear probing. */
		pos = (pos + 1) % h->table_size;
	}

	if (!found) {
		return false;
	}

	/* Remove the entry */
	free(entry->key);
	h->destructor(entry->value);
	entry->key = NULL;
	entry->value = NULL;
	entry->in_use = REMOVED;
	h->nentries--;
	h->nremoved++;

	if (!auto_resize_table(h))
		return false;

	return true;
}


/* Generic, simple cache.
 *
 * It is implemented using a hash table and manipulating it directly when
 * needed.
 *
 * It favours reads over writes, and it's very basic.
 * It IS thread-safe.
 */

struct cache {
	struct hash *hash;
	size_t size;
	pthread_rwlock_t lock;
};

struct cache *cache_create()
{
	struct cache *c;

	c = malloc(sizeof(struct cache));
	if (c == NULL)
		return NULL;

	c->hash = hash_create(NULL);
	if (c->hash == NULL) {
		free(c);
		return NULL;
	}

	pthread_rwlock_init(&c->lock, NULL);

	return c;
}

void cache_free(struct cache *c)
{
	hash_free(c->hash);
	pthread_rwlock_destroy(&c->lock);
	free(c);
}

/* Internal non-locking version of cache_invalidate(). */
static void _cache_invalidate(struct cache *c)
{
	size_t i;
	struct entry *entry;

	for (i = 0; i < c->hash->table_size; i++) {
		entry = c->hash->entries + i;
		if (entry->in_use == IN_USE) {
			free(entry->key);
			entry->key = NULL;
			entry->value = NULL;
			entry->in_use = NEVER;
		}
	}
}

bool cache_invalidate(struct cache *c)
{
	pthread_rwlock_wrlock(&c->lock);
	_cache_invalidate(c);
	pthread_rwlock_unlock(&c->lock);

	return true;
}

bool cache_resize(struct cache *c, size_t new_size)
{
	pthread_rwlock_wrlock(&c->lock);
	if (new_size > c->size) {
		if (resize_table(c->hash, new_size)) {
			c->size = new_size;
			goto success;
		}
	} else {
		/* TODO: Implement shrinking. We just invalidate everything
		 * for now, and then resize. */
		_cache_invalidate(c);

		if (resize_table(c->hash, new_size)) {
			c->size = new_size;
			goto success;
		}
	}

	pthread_rwlock_unlock(&c->lock);
	return false;

success:
	pthread_rwlock_unlock(&c->lock);
	return true;
}

struct entry *entry_for_key(struct cache *c, const char *key)
{
	size_t pos;
	struct entry *entry;

	pos = murmurhash2(key, strlen(key)) % c->hash->table_size;

	entry = c->hash->entries + pos;

	return entry;
}

bool cache_get(struct cache *c, const char *key, void **value)
{
	pthread_rwlock_rdlock(&c->lock);
	struct entry *e = entry_for_key(c, key);

	*value = NULL;

	if (e->in_use != IN_USE)
		goto miss;

	if (strcmp(key, e->key) == 0) {
		*value = e->value;
		goto hit;
	} else {
		goto miss;
	}

hit:
	pthread_rwlock_unlock(&c->lock);
	return true;

miss:
	pthread_rwlock_unlock(&c->lock);
	return false;
}


bool cache_set(struct cache *c, const char *key, void *value)
{
	pthread_rwlock_wrlock(&c->lock);
	struct entry *e = entry_for_key(c, key);

	if (e->in_use == IN_USE)
		free(e->key);

	e->in_use = IN_USE;

	e->key = strdup(key);
	e->value = value;

	pthread_rwlock_unlock(&c->lock);
	return true;
}

bool cache_del(struct cache *c, const char *key)
{
	pthread_rwlock_wrlock(&c->lock);
	struct entry *e = entry_for_key(c, key);

	if (e->in_use == IN_USE && strcmp(e->key, key) == 0) {
		free(e->key);
		e->key = NULL;
		e->value = NULL;
		e->in_use = NEVER;
		pthread_rwlock_unlock(&c->lock);
		return true;
	}

	pthread_rwlock_unlock(&c->lock);
	return false;
}

