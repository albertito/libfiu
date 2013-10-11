
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
#include "wtable.h"


struct entry {
	char *key;
	size_t key_len;

	void *value;
	bool in_use;
};

struct wtable {
	struct entry *entries;
	size_t table_size;
	size_t nentries;
	void (*destructor)(void *);
};


/* Minimum table size. */
#define MIN_SIZE 10

struct wtable *wtable_create(void (*destructor)(void *))
{
	struct wtable *t = malloc(sizeof(struct wtable));
	if (t == NULL)
		return NULL;

	t->entries = malloc(sizeof(struct entry) * MIN_SIZE);
	if (t->entries == NULL) {
		free(t);
		return NULL;
	}

	memset(t->entries, 0, sizeof(struct entry) * MIN_SIZE);

	t->table_size = MIN_SIZE;
	t->nentries = 0;
	t->destructor = destructor;

	return t;
}

void wtable_free(struct wtable *t)
{
	int i;
	struct entry *entry;

	for (i = 0; i < t->table_size; i++) {
		entry = t->entries + i;
		if (entry->in_use) {
			t->destructor(entry->value);
			free(entry->key);
		}
	}

	free(t->entries);
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

	if (exact || ws[ws_len - 1] != '*') {
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
static struct entry *find_entry(struct wtable *t, const char *key,
		bool exact, struct entry **first_free)
{
	size_t key_len;
	size_t pos;
	struct entry *entry;
	bool found_free = false;

	key_len = strlen(key);

	for (pos = 0; pos < t->table_size; pos++) {
		entry = t->entries + pos;
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
	struct entry *entry = find_entry(t, key, false, NULL);

	if (entry)
		return entry->value;
	return NULL;
}

/* Internal version of wtable_set().
 * It uses the key as-is (it won't copy it), and it won't resize the array
 * either. */
static bool _wtable_set(struct wtable *t, char *key, void *value)
{
	struct entry *entry, *first_free;

	first_free = NULL;
	entry = find_entry(t, key, true, &first_free);

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
		t->nentries++;
		return true;
	}

	/* This should never happen, as we should always have space by the
	 * time this function is called. */
	return false;
}

static bool resize_table(struct wtable *t, size_t new_size)
{
	size_t i;
	struct entry *old_entries, *e;
	size_t old_size;

	if (new_size < MIN_SIZE) {
		/* Do not resize below the minimum size. */
		return true;
	}

	old_entries = t->entries;
	old_size = t->table_size;

	t->entries = malloc(sizeof(struct entry) * new_size);
	if (t->entries == NULL)
		return false;

	memset(t->entries, 0, sizeof(struct entry) * new_size);
	t->table_size = new_size;
	t->nentries = 0;

	/* Insert the old entries into the new table. We use the internal
	 * version _wtable_set() to avoid copying the keys again. */
	for (i = 0; i < old_size; i++) {
		e = old_entries + i;
		if (e->in_use) {
			_wtable_set(t, e->key, e->value);
		}
	}

	return true;
}

bool wtable_set(struct wtable *t, const char *key, void *value)
{
	if ((t->table_size - t->nentries) <= 1) {
		/* If we only have one entry left, grow by 30%, plus one to
		 * make sure we always increase even if the percentage isn't
		 * enough. */
		if (!resize_table(t, t->table_size * 1.3 + 1))
			return false;
	}

	return _wtable_set(t, strdup(key), value);
}


bool wtable_del(struct wtable *t, const char *key)
{
	struct entry *entry;

	entry = find_entry(t, key, true, NULL);

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

	t->nentries--;

	/* Shrink if the table is less than 60% occupied. */
	if (t->table_size > MIN_SIZE &&
			(float) t->nentries / t->table_size < 0.6) {
		if (!resize_table(t, t->nentries + 3))
			return false;
	}

	return true;
}

