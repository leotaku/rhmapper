#ifndef BBTEX_RHMAPPER_H
#define BBTEX_RHMAPPER_H

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define XXH_INLINE_ALL
#include "xxhash.h"

#define RHMAPPER_GROW_FACTOR 2
#define RHMAPPER_GROW_RATIO 8 / 10
#define RHMAPPER_EMPTY_VALUE -1

#define RHMAPPER_HASH(data, size) XXH64(data, size, 0)
#define RHMAPPER_MEASURE(index, hash, capacity) \
  ((capacity + index % capacity - hash % capacity) % capacity)
#define RHMAPPER_RANK(new, old)             \
  (RHMAPPER_MEASURE(index, new, capacity) > \
   RHMAPPER_MEASURE(index, old, capacity))
#define RHMAPPER_NEXT(index) index + 1

void *rhmapper_calloc(size_t n, size_t size) {
  void *result = calloc(n, size);
  assert(result);
  return result;
}

typedef struct rhmapper rhmapper_t;
typedef struct rhmapper_string rhmapper_string_t;
typedef struct rhmapper_kv_remote rhmapper_kv_remote_t;
typedef struct rhmapper_kv rhmapper_kv_t;

struct rhmapper {
  size_t size;
  size_t capacity;
  rhmapper_kv_t *array;
};

struct rhmapper_string {
  size_t size;
  char *data;
};

struct rhmapper_kv_remote {
  size_t value;
  rhmapper_string_t key;
};

struct rhmapper_kv {
  size_t hash;
  rhmapper_kv_remote_t *remote;
};

rhmapper_t *rhmapper_create(size_t capacity) {
  assert(capacity >= 1);

  rhmapper_t *rh;
  rh = rhmapper_calloc(1, sizeof(rhmapper_t));
  rh->size = 0;
  rh->capacity = capacity;
  rh->array = rhmapper_calloc(capacity, sizeof(rhmapper_kv_t));

  return rh;
}

void rhmapper_destroy(rhmapper_t *rh) {
  const size_t size = rh->capacity;
  for (size_t i = 0; i < size; i++) {
    rhmapper_kv_t it = rh->array[i];
    if (it.remote != NULL) {
      free(it.remote->key.data);
      free(it.remote);
    }
  }
  free(rh->array);
  free(rh);
}

void rhmapper_internal_set(rhmapper_t *rh, rhmapper_kv_t kv, size_t index) {
  size_t capacity = rh->capacity;
  for (;;) {
    rhmapper_kv_t stored = rh->array[index % capacity];
    if (stored.remote == NULL) {
      rh->array[index % capacity] = kv;
      return;
    } else if (RHMAPPER_RANK(kv.hash, stored.hash)) {
      rhmapper_kv_t tmp = kv;
      kv = stored;
      rh->array[index % capacity] = tmp;
    }
    index = RHMAPPER_NEXT(index);
  }
}

void rhmapper_grow(rhmapper_t *rh, size_t capacity) {
  assert(capacity >= rh->size);
  size_t old_capacity = rh->capacity;
  rhmapper_kv_t *old_array = rh->array;
  rh->array = rhmapper_calloc(capacity, sizeof(rhmapper_kv_t));
  rh->capacity = capacity;
  for (size_t i = old_capacity; i > 0; i--) {
    rhmapper_kv_t it = old_array[i - 1];
    if (it.remote != NULL) {
      rhmapper_internal_set(rh, it, it.hash);
    }
  }
  free(old_array);
}

size_t rhmapper_put(rhmapper_t *rh, char *key, size_t size) {
  size_t hash = RHMAPPER_HASH(key, size);
  size_t capacity = rh->capacity;
  size_t index = hash;
  for (;;) {
    rhmapper_kv_t it = rh->array[index % capacity];
    if (it.remote == NULL) {
      char *data = rhmapper_calloc(size, sizeof(char));
      memcpy(data, key, size);
      rhmapper_kv_remote_t *remote =
          rhmapper_calloc(1, sizeof(rhmapper_kv_remote_t));
      remote->value = rh->size++;
      remote->key.data = data;
      remote->key.size = size;
      rhmapper_kv_t kv = {
          .remote = remote,
          .hash = hash,
      };
      if (rh->size > capacity * RHMAPPER_GROW_RATIO) {
        rhmapper_grow(rh, rh->capacity * RHMAPPER_GROW_FACTOR);
      }
      rhmapper_internal_set(rh, kv, kv.hash);
      return kv.remote->value;
    } else if (
        it.hash == hash && it.remote->key.size == size &&
        !memcmp(key, it.remote->key.data, size)) {
      return it.remote->value;
    } else {
      index = RHMAPPER_NEXT(index);
    }
  }
}

size_t rhmapper_get(rhmapper_t *rh, char *key, size_t size) {
  size_t hash = RHMAPPER_HASH(key, size);
  size_t capacity = rh->capacity;
  size_t index = hash;
  for (;;) {
    rhmapper_kv_t it = rh->array[index % capacity];
    if (it.remote == NULL) {
      return RHMAPPER_EMPTY_VALUE;
    } else if (RHMAPPER_RANK(hash, it.hash)) {
      return RHMAPPER_EMPTY_VALUE;
    } else if (
        it.hash == hash && it.remote->key.size == size &&
        !memcmp(key, it.remote->key.data, size)) {
      return it.remote->value;
    } else {
      index = RHMAPPER_NEXT(index);
    }
  }
}

#endif
