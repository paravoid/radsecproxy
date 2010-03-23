/*
 * Copyright (C) 2008 Stig Venaas <venaas@uninett.no>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 */

#ifndef SYS_SOLARIS9
#include <stdint.h>
#endif

struct hash {
    struct list *hashlist;
    pthread_mutex_t mutex;
};

struct hash_entry {
    void *key;
    uint32_t keylen;
    void *data;
    struct list_node *next; /* used when walking through hash */
};

/* allocates and initialises hash structure; returns NULL if malloc fails */
struct hash *hash_create();

/* frees all memory associated with the hash */
void hash_destroy(struct hash *hash);

/* insert entry in hash; returns 1 if ok, 0 if malloc fails */
int hash_insert(struct hash *hash, void *key, uint32_t keylen, void *data);

/* reads entry from hash */
void *hash_read(struct hash *hash, void *key, uint32_t keylen);

/* extracts (read and remove) entry from hash */
void *hash_extract(struct hash *hash, void *key, uint32_t keylen);

/* returns first entry */
struct hash_entry *hash_first(struct hash *hash);

/* returns the next entry after the argument */
struct hash_entry *hash_next(struct hash_entry *entry);

/* Local Variables: */
/* c-file-style: "stroustrup" */
/* End: */
