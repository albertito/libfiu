
/*
 * Wildcard table.
 *
 * This is a simple key-value table that associates a key (\0-terminated
 * string) with a value (void *).
 *
 * The keys can end in a wildcard ('*'), and then a lookup will match them as
 * expected.
 *
 * The current implementation is a very simple dynamic array, which is good
 * enough for a small number of elements (< 100), but we should optimize this
 * in the future, in particular to speed up lookups.
 *
 * If more than one entry matches, we do not guarantee any order, although
 * this may change in the future.
 */

#include <sys/types.h>		/* for size_t */
#include <stdint.h>		/* for [u]int*_t */
#include <stdbool.h>		/* for bool */
#include <stdlib.h>		/* for malloc() */
#include <string.h>		/* for memcpy()/memcmp() */
#include <stdio.h>		/* snprintf() */

#include "hash.h"
#include "wtable.h"


/* Entry of the wildcard array. */
struct wentry {
	char *key;
	size_t key_len;

	void *value;
	bool in_use;
};

struct wtable {
	/* Final (non-wildcard) entries are kept in this hash. */
	hash_t *finals;

	/* Wildcarded entries are kept in this dynamic array. */
	struct wentry *wildcards;
	size_t ws_size;
	size_t ws_used_count;

	/* And we keep a cache of lookups into the wildcards array. */
	cache_t *wcache;

	void (*destructor)(void *);
};


/* Minimum table size. */
#define MIN_SIZE 10


struct wtable *wtable_create(void (*destructor)(void *))
{
	struct wtable *t = malloc(sizeof(struct wtable));
	if (t == NULL)
		return NULL;

	t->wildcards = NULL;
	t->wcache = NULL;

	t->finals = hash_create(destructor);
	if (t->finals == NULL)
		goto error;

	t->wildcards = malloc(sizeof(struct wentry) * MIN_SIZE);
	if (t->wildcards == NULL)
		goto error;

	memset(t->wildcards, 0, sizeof(struct wentry) * MIN_SIZE);

	t->wcache = cache_create();
	if (t->wcache == NULL)
		goto error;

	t->ws_size = MIN_SIZE;
	t->ws_used_count = 0;
	t->destructor = destructor;

	return t;

error:
	if (t->finals)
		hash_free(t->finals);
	if (t->wcache)
		cache_free(t->wcache);
	free(t->wildcards);
	free(t);
	return NULL;
}

void wtable_free(struct wtable *t)
{
	int i;
	struct wentry *entry;

	hash_free(t->finals);
	cache_free(t->wcache);

	for (i = 0; i < t->ws_size; i++) {
		entry = t->wildcards + i;
		if (entry->in_use) {
			t->destructor(entry->value);
			free(entry->key);
		}
	}

	free(t->wildcards);
	free(t);
}

/* Return the last position where s1 and s2 match. */
static unsigned int strlast(const char *s1, const char *s2)
{
	unsigned int i = 0;

	while (*s1 != '\0' && *s2 != '\0' && *s1 == *s2) {
		i++;
		s1++;
		s2++;
	}

	return i;
}


/* True if s is a wildcarded string, False otherwise. */
static bool is_wildcard(const char *s, size_t len)
{
	/* Note we only support wildcards at the end of the string for now. */
	return s[len - 1] == '*';
}

/* Checks if ws matches s.
 *
 * ws is a "wildcard string", which can end in a '*', in which case we compare
 * only up to that position, to do a wildcard matching.
 */
static bool ws_matches_s(
		const char *ws, size_t ws_len,
		const char *s, size_t s_len,
		bool exact)
{
	if (ws == NULL || s == NULL)
		return false;

	if (exact || !is_wildcard(ws, ws_len)) {
		/* Exact match */
		if (ws_len != s_len)
			return false;
		return strcmp(ws, s) == 0;
	} else {
		/* Inexact match */
		return strlast(ws, s) >= ws_len - 1;
	}
}

/* Find the entry matching the given key.
 *
 * If exact == true, then the key must match exactly (no wildcard matching).
 * If first_free is not NULL, then it will be made to point to the first free
 * entry found during the lookup.
 *
 * Returns a pointer to the entry, or NULL if not found.
 */
static struct wentry *wildcards_find_entry(struct wtable *t, const char *key,
		bool exact, struct wentry **first_free)
{
	size_t key_len;
	size_t pos;
	struct wentry *entry;
	bool found_free = false;

	key_len = strlen(key);

	for (pos = 0; pos < t->ws_size; pos++) {
		entry = t->wildcards + pos;
		if (!entry->in_use) {
			if (!found_free && first_free) {
				*first_free = entry;
				found_free = true;
			}
			continue;

		} else if (ws_matches_s(entry->key, entry->key_len,
				key, key_len, exact)) {
			/* The key matches. */
			return entry;
		}
	}

	/* No match. */
	return NULL;
}

void *wtable_get(struct wtable *t, const char *key)
{
	void *value;
	struct wentry *entry;

	/* Do an exact lookup first. */
	value = hash_get(t->finals, key);
	if (value)
		return value;

	/* Then see if we can find it in the wcache */
	if (cache_get(t->wcache, key, &value))
		return value;

	/* And then walk the wildcards array. */
	entry = wildcards_find_entry(t, key, false, NULL);
	if (entry) {
		cache_set(t->wcache, key, entry->value);
		return entry->value;
	}

	/* Cache the negative result as well. */
	cache_set(t->wcache, key, NULL);

	return NULL;
}

/* Set on our wildcards table.
 * For internal use only.
 * It uses the key as-is (it won't copy it), and it won't resize the array
 * either. */
static bool wildcards_set(struct wtable *t, char *key, void *value)
{
	struct wentry *entry, *first_free;

	first_free = NULL;
	entry = wildcards_find_entry(t, key, true, &first_free);

	if (entry) {
		/* Found a match, override the value. */
		free(entry->key);
		entry->key = key;
		t->destructor(entry->value);
		entry->value = value;
		return true;
	} else if (first_free) {
		/* We are inserting a new entry. */
		first_free->key = key;
		first_free->key_len = strlen(key);
		first_free->value = value;
		first_free->in_use = true;
		t->ws_used_count++;
		return true;
	}

	/* This should never happen, as we should always have space by the
	 * time this function is called. */
	return false;
}

static bool resize_table(struct wtable *t, size_t new_size)
{
	size_t i;
	struct wentry *old_wildcards, *e;
	size_t old_size;

	if (new_size < MIN_SIZE) {
		/* Do not resize below the minimum size. */
		return true;
	}

	old_wildcards = t->wildcards;
	old_size = t->ws_size;

	t->wildcards = malloc(sizeof(struct wentry) * new_size);
	if (t->wildcards == NULL)
		return false;

	memset(t->wildcards, 0, sizeof(struct wentry) * new_size);
	t->ws_size = new_size;
	t->ws_used_count = 0;

	/* Insert the old entries into the new table. */
	for (i = 0; i < old_size; i++) {
		e = old_wildcards + i;
		if (e->in_use) {
			wildcards_set(t, e->key, e->value);
		}
	}

	free(old_wildcards);

	/* Keep the cache the same size as our table, which works reasonably
	 * well in practise */
	cache_resize(t->wcache, new_size);

	return true;
}

bool wtable_set(struct wtable *t, const char *key, void *value)
{
	if (is_wildcard(key, strlen(key))) {
		if ((t->ws_size - t->ws_used_count) <= 1) {
			/* If we only have one entry left, grow by 30%, plus
			 * one to make sure we always increase even if the
			 * percentage isn't enough. */
			if (!resize_table(t, t->ws_size * 1.3 + 1))
				return false;
		}

		/* Invalidate the cache. We could be smart and walk it,
		 * removing only the negative hits, but it's also more
		 * expensive */
		cache_invalidate(t->wcache);

		return wildcards_set(t, strdup(key), value);
	} else {
		return hash_set(t->finals, key, value);
	}
}


bool wtable_del(struct wtable *t, const char *key)
{
	struct wentry *entry;

	if (is_wildcard(key, strlen(key))) {
		entry = wildcards_find_entry(t, key, true, NULL);

		if (!entry) {
			/* Key not found. */
			return false;
		}

		/* Mark the entry as free. */
		free(entry->key);
		entry->key = NULL;
		entry->key_len = 0;
		t->destructor(entry->value);
		entry->value = NULL;
		entry->in_use = false;

		t->ws_used_count--;

		/* Shrink if the table is less than 60% occupied. */
		if (t->ws_size > MIN_SIZE &&
				(float) t->ws_used_count / t->ws_size < 0.6) {
			if (!resize_table(t, t->ws_used_count + 3))
				return false;
		}

		/* Invalidate the cache. We could be smart and walk it,
		 * removing only the positive hits, but it's also more
		 * expensive */
		cache_invalidate(t->wcache);

		return true;
	} else {
		return hash_del(t->finals, key);
	}
}

