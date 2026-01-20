#include "cache.h"

LRUCache* init_cache() {
    LRUCache *cache = malloc(sizeof(LRUCache));
    cache->count = 0;
    pthread_mutex_init(&cache->lock, NULL);
    return cache;
}

CacheBlock* get_block(LRUCache *cache, const char *block_id) {
    pthread_mutex_lock(&cache->lock);
    for (int i = 0; i < cache->count; i++) {
        if (strcmp(cache->blocks[i]->block_id, block_id) == 0 && cache->blocks[i]->valid) {
            pthread_mutex_unlock(&cache->lock);
            return cache->blocks[i];
        }
    }
    pthread_mutex_unlock(&cache->lock);
    return NULL;
}

void invalidate_block(LRUCache *cache, const char *block_id) {
    for (int i = 0; i < cache->count; i++) {
        if (strcmp(cache->blocks[i]->block_id, block_id) == 0) {
            cache->blocks[i]->valid = 0;
            printf("🧹 Блок %s инвалидирован\n", block_id);
            return;
        }
    }
}

void put_block(LRUCache *cache, const char *block_id, char *data, size_t size, unsigned char xor_key, unsigned int access_rights) {
    pthread_mutex_lock(&cache->lock);

    for (int i = 0; i < cache->count; i++) {
        if (strcmp(cache->blocks[i]->block_id, block_id) == 0) {
            if (cache->blocks[i]->access_rights != access_rights) {
                printf("⚠️  Права изменились, блок %s инвалидирован\n", block_id);
                invalidate_block(cache, block_id);
            } else {
                printf("♻️  Блок %s перезаписан без изменений прав\n", block_id);
            }
            pthread_mutex_unlock(&cache->lock);
            return;
        }
    }

    if (cache->count >= CACHE_CAPACITY) {
        printf("❗ Кэш полон, удаляем старый блок %s\n", cache->blocks[0]->block_id);
        free(cache->blocks[0]->data);
        free(cache->blocks[0]);
        for (int i = 1; i < cache->count; i++)
            cache->blocks[i - 1] = cache->blocks[i];
        cache->count--;
    }

    CacheBlock *block = malloc(sizeof(CacheBlock));
    strncpy(block->block_id, block_id, sizeof(block->block_id));
    block->data = malloc(size);
    for (size_t i = 0; i < size; i++)
        block->data[i] = data[i] ^ xor_key;
    block->size = size;
    block->xor_key = xor_key;
    block->access_rights = access_rights;
    block->valid = 1;

    cache->blocks[cache->count++] = block;
    printf("💾 Блок %s добавлен в кэш (шифр XOR 0x%02X, права 0x%X)\n", block_id, xor_key, access_rights);

    pthread_mutex_unlock(&cache->lock);
}

void free_cache(LRUCache *cache) {
    for (int i = 0; i < cache->count; i++) {
        free(cache->blocks[i]->data);
        free(cache->blocks[i]);
    }
    pthread_mutex_destroy(&cache->lock);
    free(cache);
}
