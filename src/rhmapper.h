#ifndef _BBTEX_RHMAPPER_H_
#define _BBTEX_RHMAPPER_H_

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define XXH_INLINE_ALL
#include "xxhash.h"

#define RHMAPPER_GROW_FACTOR 2
#define RHMAPPER_GROW_RATIO 8 / 10
#define RHMAPPER_EMPTY_VALUE -1

size_t rhmapper_hash(const void *data, size_t size) {
  XXH32_hash_t hash = XXH32(data, size, 0);
  return hash;
}

typedef struct rhmapper rhmapper_t;
typedef struct rhmapper_string rhmapper_string_t;
typedef struct rhmapper_kv rhmapper_kv_t;

struct rhmapper {
  size_t size;
  size_t capacity;
  rhmapper_kv_t *array;
};

struct rhmapper_string {
  char *data;
  size_t size;
};

struct rhmapper_kv {
  rhmapper_string_t key;
  size_t value;
  size_t hash;
};

rhmapper_t *rhmapper_create(size_t capacity) {
  assert(capacity >= 1);

  rhmapper_t *rh;
  rh = calloc(1, sizeof(rhmapper_t));
  rh->size = 0;
  rh->capacity = capacity;
  rh->array = calloc(capacity, sizeof(rhmapper_kv_t));

  return rh;
}

void rhmapper_destroy(rhmapper_t *rh) {
  const size_t size = rh->capacity;
  for (size_t i = 0; i < size; i++) {
    rhmapper_kv_t it = rh->array[i];
    if (it.key.data != NULL) {
      free(it.key.data);
    }
  }
  free(rh->array);
  free(rh);
}

size_t
rhmapper_internal_set(rhmapper_t *rh, rhmapper_kv_t kv, size_t index) {
  size_t capacity = rh->capacity;
  for (;;) {
    rhmapper_kv_t stored = rh->array[index % capacity];
    if (stored.key.data == NULL) {
      rh->array[index % capacity] = kv;
      return kv.value;
    } else if (kv.hash % capacity < stored.hash % capacity) {
      rhmapper_kv_t tmp = kv;
      kv = stored;
      rh->array[index % capacity] = tmp;
    }
    index++;
  }
}

size_t rhmapper_internal_put(rhmapper_t *rh, char *key, size_t size) {
  size_t hash = rhmapper_hash(key, size);
  size_t capacity = rh->capacity;
  size_t index = hash;
  for (;;) {
    rhmapper_kv_t it = rh->array[index % capacity];
    if (it.key.data == NULL || hash % capacity < it.hash % capacity) {
      char *data = calloc(size, sizeof(char));
      memcpy(data, key, size);
      rhmapper_kv_t kv = {
          .value = rh->size++,
          .key.data = data,
          .key.size = size,
          .hash = hash,
      };
      return rhmapper_internal_set(rh, kv, index);
    } else if (it.key.size == size && !memcmp(key, it.key.data, size)) {
      return it.value;
    } else {
      index++;
    }
  }
}

void rhmapper_grow(rhmapper_t *rh, size_t capacity) {
  assert(capacity > rh->capacity);
  size_t old_capacity = rh->capacity;
  rhmapper_kv_t *old_array = rh->array;
  rh->array = calloc(capacity, sizeof(rhmapper_kv_t));
  rh->capacity = capacity;
  for (size_t i = 0; i < old_capacity; i++) {
    rhmapper_kv_t it = old_array[i];
    if (it.key.data != NULL) {
      rhmapper_internal_set(rh, it, it.hash);
    }
  }
  free(old_array);
}

size_t rhmapper_put(rhmapper_t *rh, char *key, size_t size) {
  if (rh->size > rh->capacity * RHMAPPER_GROW_RATIO) {
    rhmapper_grow(rh, rh->capacity * RHMAPPER_GROW_FACTOR);
  }
  return rhmapper_internal_put(rh, key, size);
}

size_t rhmapper_get(rhmapper_t *rh, char *key, size_t size) {
  size_t hash = rhmapper_hash(key, size);
  size_t capacity = rh->capacity;
  size_t index = hash;
  for (;;) {
    rhmapper_kv_t it = rh->array[index % capacity];
    if (it.key.data == NULL) {
      return RHMAPPER_EMPTY_VALUE;
    } else if (hash % capacity < it.hash % capacity) {
      return RHMAPPER_EMPTY_VALUE;
    } else if (it.key.size == size && !memcmp(key, it.key.data, size)) {
      return it.value;
    } else {
      index++;
    }
  }
}

#endif
