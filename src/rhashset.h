#ifndef _BBTEX_RHASHSET_H_
#define _BBTEX_RHASHSET_H_

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define XXH_INLINE_ALL
#include "xxhash.h"

#define RHASHSET_GROW_FACTOR 2
#define RHASHSET_GROW_RATIO 8 / 10
#define RHASHSET_EMPTY_VALUE -1

size_t rhashset_hash(const void *data, size_t size) {
  XXH32_hash_t hash = XXH32(data, size, 0);
  return hash;
}

typedef struct rhashset rhashset_t;
typedef struct rhashset_string rhashset_string_t;
typedef struct rhashset_kv rhashset_kv_t;

struct rhashset {
  size_t size;
  size_t capacity;
  rhashset_kv_t *array;
};

struct rhashset_string {
  char *data;
  size_t size;
};

struct rhashset_kv {
  rhashset_string_t key;
  size_t value;
  size_t hash;
  size_t poverty;
};

rhashset_t *rhashset_create(size_t capacity) {
  assert(capacity >= 1);

  rhashset_t *hset;
  hset = calloc(1, sizeof(rhashset_t));
  hset->size = 0;
  hset->capacity = capacity;
  hset->array = calloc(capacity, sizeof(rhashset_kv_t));

  return hset;
}

void rhashset_destroy(rhashset_t *hset) {
  const size_t size = hset->capacity;
  for (size_t i = 0; i < size; i++) {
    rhashset_kv_t it = hset->array[i];
    if (it.key.data != NULL) {
      free(it.key.data);
    }
  }
  free(hset->array);
  free(hset);
}

size_t rhashset_internal_force_set(rhashset_t *hset, rhashset_kv_t kv) {
  for (;;) {
    size_t index = (kv.hash + kv.poverty) % hset->capacity;
    rhashset_kv_t stored = hset->array[index];
    if (stored.key.data == NULL) {
      hset->array[index] = kv;
      hset->size++;
      return kv.value;
    } else if (kv.poverty > stored.poverty) {
      rhashset_kv_t tmp = kv;
      kv = stored;
      hset->array[index] = tmp;
    }
    kv.poverty++;
  }
}

size_t rhashset_internal_maybe_set(
    rhashset_t *hset, char *key, size_t size, size_t val) {
  size_t hash = rhashset_hash(key, size);
  size_t poverty = 0;
  for (;;) {
    rhashset_kv_t it = hset->array[(hash + poverty) % hset->capacity];
    if (it.key.data == NULL || poverty > it.poverty) {
      char *data = calloc(size, sizeof(char));
      memcpy(data, key, size);
      rhashset_kv_t kv = {
          .value = val,
          .key.data = data,
          .key.size = size,
          .hash = hash,
          .poverty = poverty,
      };
      return rhashset_internal_force_set(hset, kv);
    } else if (it.key.size == size && !memcmp(key, it.key.data, size)) {
      return it.value;
    } else {
      poverty++;
    }
  }
}

void rhashset_grow(rhashset_t *hset, size_t capacity) {
  assert(capacity > hset->capacity);
  size_t old_capacity = hset->capacity;
  rhashset_kv_t *old_array = hset->array;
  hset->array = calloc(capacity, sizeof(rhashset_kv_t));
  hset->capacity = capacity;
  hset->size = 0;
  for (size_t i = 0; i < old_capacity; i++) {
    rhashset_kv_t it = old_array[i];
    if (it.key.data != NULL) {
      it.poverty = 0;
      rhashset_internal_force_set(hset, it);
    }
  }
  free(old_array);
}

size_t rhashset_put(rhashset_t *hset, char *key, size_t size) {
  if (hset->size > hset->capacity * RHASHSET_GROW_RATIO) {
    rhashset_grow(hset, hset->capacity * RHASHSET_GROW_FACTOR);
  }
  return rhashset_internal_maybe_set(hset, key, size, hset->size);
}

size_t rhashset_get(rhashset_t *hset, char *key, size_t size) {
  size_t hash = rhashset_hash(key, size);
  size_t poverty = 0;
  for (;;) {
    rhashset_kv_t it = hset->array[(hash + poverty) % hset->capacity];
    if (it.key.data == NULL) {
      return RHASHSET_EMPTY_VALUE;
    } else if (poverty > it.poverty) {
      return RHASHSET_EMPTY_VALUE;
    } else if (it.key.size == size && !memcmp(key, it.key.data, size)) {
      return it.value;
    } else {
      poverty++;
    }
  }
}

#endif
