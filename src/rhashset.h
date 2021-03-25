#ifndef _BBTEX_RHASHSET_H_
#define _BBTEX_RHASHSET_H_

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define XXH_INLINE_ALL
#include "xxhash.h"

#define RHASHSET_GROW_FACTOR 4
#define RHASHSET_GROW_RATIO 6 / 10
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
  rhashset_string_t *reverse;
  rhashset_kv_t *array;
};

struct rhashset_string {
  char *data;
  size_t size;
};

struct rhashset_kv {
  rhashset_string_t key;
  size_t value;
};

rhashset_t *rhashset_create(size_t capacity) {
  assert(capacity >= 1);

  rhashset_t *hset;
  hset = calloc(1, sizeof(rhashset_t));
  hset->size = 0;
  hset->capacity = capacity;
  hset->array = calloc(capacity, sizeof(rhashset_kv_t));
  hset->reverse = calloc(capacity, sizeof(char *));

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
  free(hset->reverse);
  free(hset->array);
  free(hset);
}

size_t
rhashset_internal_set(rhashset_t *hset, char *key, size_t size, size_t val) {
  size_t index = rhashset_hash(key, size) % hset->capacity;
  for (;;) {
    rhashset_kv_t it = hset->array[index];
    if (it.key.data == NULL) {
      char *key_ptr = calloc(size, sizeof(char));
      memcpy(key_ptr, key, size);
      rhashset_kv_t kv = {
          .value = val,
          .key.data = key_ptr,
          .key.size = size,
      };
      hset->reverse[val] = kv.key;
      hset->array[index] = kv;
      return val;
    } else if (it.key.size == size && !memcmp(key, it.key.data, size)) {
      return it.value;
    } else {
      index = (index + 1) % hset->capacity;
    }
  }
}

void rhashset_grow(rhashset_t *hset, size_t capacity) {
  assert(capacity > hset->capacity);
  size_t old_capacity = hset->capacity;
  rhashset_kv_t *old_array = hset->array;
  hset->reverse = realloc(hset->reverse, sizeof(char *) * capacity);
  hset->array = calloc(capacity, sizeof(rhashset_kv_t));
  hset->capacity = capacity;
  for (size_t i = 0; i < old_capacity; i++) {
    rhashset_kv_t it = old_array[i];
    if (it.key.data != NULL) {
      rhashset_internal_set(hset, it.key.data, it.key.size, it.value);
      free(it.key.data);
    }
  }
}

size_t rhashset_put(rhashset_t *hset, char *key, size_t size) {
  if (hset->size > hset->capacity * RHASHSET_GROW_RATIO) {
    rhashset_grow(hset, hset->capacity * RHASHSET_GROW_FACTOR);
  }
  return rhashset_internal_set(hset, key, size, hset->size++);
}

size_t rhashset_get(rhashset_t *hset, char *key, size_t size) {
  size_t index = rhashset_hash(key, size) % hset->capacity;
  for (;;) {
    rhashset_kv_t it = hset->array[index];
    if (it.key.data == NULL) {
      return RHASHSET_EMPTY_VALUE;
    } else if (it.key.size == size && !memcmp(key, it.key.data, size)) {
      return it.value;
    } else {
      index = (index + 1) % hset->capacity;
    }
  }
}

const rhashset_string_t rhashset_rev(rhashset_t *hset, size_t key) {
  if (key == RHASHSET_EMPTY_VALUE || key >= hset->size) {
    return (rhashset_string_t){
        .data = NULL,
        .size = 0,
    };
  } else {
    return hset->reverse[key];
  }
}

#endif
