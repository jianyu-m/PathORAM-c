//
// Created by jyjia on 2016/4/29.
//

#ifndef PATHORAM_BUCKET_H
#define PATHORAM_BUCKET_H

#include "crypt.h"

#define ORAM_FILE_FORMAT "ORAM_BUCKET_%d.bucket"

typedef struct {
    unsigned int address[ORAM_BUCKET_REAL];
    unsigned int offset[ORAM_BUCKET_SIZE];
    unsigned char encrypt_key[ORAM_CRYPT_KEY_LEN];
} oram_bucket_metadata;

#define ORAM_CRYPT_META_SIZE sizeof(oram_bucket_metadata) + ORAM_CRYPT_OVERHEAD
#define ORAM_CRYPT_DATA_SIZE ORAM_BLOCK_SIZE + ORAM_CRYPT_OVERHEAD

typedef struct {
    unsigned int read_counter;
    _bool valid_bits[ORAM_BUCKET_SIZE];
    unsigned char encrypt_metadata[ORAM_CRYPT_META_SIZE];
    unsigned char data[ORAM_BUCKET_SIZE][ORAM_CRYPT_DATA_SIZE];
} oram_bucket;

typedef struct {
    unsigned int read_counter;
    _bool valid_bits[ORAM_BUCKET_SIZE];
    unsigned char encrypt_metadata[ORAM_BUCKET_ENCRYPT_META];
} oram_bucket_encrypted_metadata;

typedef struct {
    int mem_counter;
    oram_bucket *bucket_list[ORAM_TOTAL_BUCKET];
    unsigned char* read_block_list[ORAM_TREE_DEPTH];
    unsigned char return_block[ORAM_BLOCK_SIZE];
} storage_ctx;

int read_bucket_from_file(int bucket_id, storage_ctx *ctx);

int write_bucket_to_file(int bucket_id, storage_ctx *ctx, int remain_in_mem);

int flush_buckets(storage_ctx *ctx, int remain_in_mem);

oram_bucket* get_bucket(int bucket_id, storage_ctx *ctx);

int invalidate_bucket_offset(int bucket_id, int bucket_offset, storage_ctx *ctx);

#endif //PATHORAM_BUCKET_H