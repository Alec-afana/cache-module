#ifndef CACHE_H
#define CACHE_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#define CACHE_CAPACITY 10

typedef struct {
    char block_id[64];
    char *data;
    size_t size;
    unsigned char xor_key;
    unsigned int access_rights; // права доступа
    int valid;
} CacheBlock;

typedef struct {
    CacheBlock *blocks[CACHE_CAPACITY];
    int count;
    pthread_mutex_t lock;
} LRUCache;

// функции кэша
LRUCache* init_cache();
CacheBlock* get_block(LRUCache *cache, const char *block_id);
void put_block(LRUCache *cache, const char *block_id, char *data, size_t size, unsigned char xor_key, unsigned int access_rights);
void invalidate_block(LRUCache *cache, const char *block_id);
void free_cache(LRUCache *cache);

#endif
