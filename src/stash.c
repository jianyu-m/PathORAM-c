//
// Created by jyjia on 2016/4/30.
//

#include <pthread.h>
#include "stash.h"
#include "client.h"

int init_stash(client_stash *stash, int size) {
    stash->address_to_stash = NULL;
    stash->bucket_to_stash = calloc(size, sizeof(stash_block *));
    stash->bucket_to_stash_count = calloc(size, sizeof(int));
    pthread_mutex_init(&stash->stash_mutex, NULL);
    return 1;
}


//Still some issue
void add_to_stash(client_stash *stash, stash_block *block) {
    stash_block *ne;
    int i;
    pthread_mutex_lock(&stash->stash_mutex);
    HASH_FIND_INT(stash->address_to_stash, &block->address, ne);
    //Does not add into hash table when exists
    if (ne != NULL) {
        log_f("already %d", block->address);
        if (ne->evict_count < client_t.backup_count) {
            ne->bucket_id[ne->evict_count++] = block->bucket_id[0];
            LL_APPEND(stash->bucket_to_stash[block->bucket_id[0]], ne);
        }
        pthread_mutex_unlock(&stash->stash_mutex);
        return;
    }
    HASH_ADD_INT(stash->address_to_stash, address, block);
    for (i = 0;i < block->evict_count;i++) {
        LL_APPEND(stash->bucket_to_stash[block->bucket_id[i]], block);
        stash->bucket_to_stash_count[block->bucket_id[i]]++;
    }
    pthread_mutex_unlock(&stash->stash_mutex);
}

//return remove block
int find_remove_by_bucket(client_stash *stash, int bucket_id, int max, stash_block *block_list[]) {
    //TODO insure the first evict_bucket_id is in the stash
    pthread_mutex_lock(&stash->stash_mutex);
    int delete_max = stash->bucket_to_stash_count[bucket_id];
    stash_block *now, *next = stash->bucket_to_stash[bucket_id];
    int i = 0, j;
    if (delete_max > max)
        delete_max = max;
    for (;i < delete_max; i++) {
        now = next;
        next = now->next_l;
        block_list[i] = now;
        if (now->evict_count == 1) {
            HASH_DEL(stash->address_to_stash, now);
            LL_DELETE(stash->bucket_to_stash[bucket_id], now);
            stash->bucket_to_stash_count[bucket_id]--;
            if (now->write_after_evict == 1) {
                now->evict_count = client_t.backup_count;
                for (j = 0; j < client_t.backup_count; i++) {
                    LL_APPEND(stash->bucket_to_stash[now->bucket_id[j]], now);
                    stash->bucket_to_stash_count[now->bucket_id[j]]++;
                    now->write_after_evict = 0;
                }
            }
        }
        now->evict_count--;
        LL_DELETE(stash->bucket_to_stash[bucket_id], now);
        stash->bucket_to_stash_count[bucket_id]--;
    }
    pthread_mutex_unlock(&stash->stash_mutex);
    return delete_max;
}

//TODO During delete, others may come
stash_block* find_edit_by_address(client_stash *stash, int address, oram_access_op op, unsigned char data[]) {
    stash_block *return_block;
    pthread_mutex_lock(&stash->stash_mutex);
    HASH_FIND_INT(stash->address_to_stash, &address, return_block);
    if (return_block != NULL) {
        if (op == ORAM_ACCESS_WRITE) {
            memcpy(return_block->data, data, ORAM_BLOCK_SIZE);
            if (return_block->evict_count < client_t.backup_count)
                return_block->write_after_evict = 1;
        }
        else
            memcpy(data, return_block->data, ORAM_BLOCK_SIZE);
//        HASH_DEL(stash->address_to_stash, return_block);
//        for (i = 0; i < return_block->evict_count; i++) {
//            LL_DELETE(stash->bucket_to_stash[return_block->bucket_id[i]], return_block);
//            stash->bucket_to_stash_count[return_block->bucket_id[i]]--;
//        }
    }
    pthread_mutex_unlock(&stash->stash_mutex);
    return return_block;
}