#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define XXH_INLINE_ALL
#include "xxhash.h"
#include "rhashset.h"

#define RHASHSET_GROW_FACTOR 4
#define RHASHSET_GROW_RATIO 6 / 10

struct rhashset {
  size_t size;
  size_t capacity;
  rhashset_item_t *array;
  const char **reverse;
};

struct rhashset_item {
  size_t value;
  size_t key_size;
  char *key;
};

rhashset_t *rhashset_create(size_t capacity) {
  assert(capacity >= 1);
  rhashset_t *hset;
  hset = (rhashset_t *)calloc(1, sizeof(rhashset_t));
  hset->size = 0;
  hset->capacity = capacity;
  hset->array = (rhashset_item_t *)calloc(capacity, sizeof(rhashset_item_t));
  hset->reverse = (const char **)calloc(capacity, sizeof(char *));

  return hset;
}

void rhashset_destroy(rhashset_t *hset) {
  const size_t size = hset->capacity;
  for (size_t i = 0; i < size; i++) {
    rhashset_item_t it = hset->array[i];
    if (it.value != 0) {
      free(it.key);
    }
  }
  free(hset->reverse);
  free(hset->array);
  free(hset);
}

void rhashset_grow(rhashset_t *hset, size_t capacity) {
  assert(capacity > hset->capacity);
  size_t old_capacity = hset->capacity;
  rhashset_item_t *old_array = hset->array;
  hset->reverse = (const char **)realloc(hset->reverse, sizeof(char *) * capacity);
  hset->array = (rhashset_item_t *)calloc(capacity, sizeof(rhashset_item_t));
  hset->capacity = capacity;
  for (size_t i = 0; i < old_capacity; i++) {
    rhashset_item_t it = old_array[i];
    if (it.value != 0) {
      rhashset_unsafe_set(hset, it.key, it.key_size, it.value);
      free(it.key);
    }
  }
}

size_t rhashset_hash(const void *data, size_t size) {
  XXH32_hash_t hash = XXH32(data, size, 0);
  return hash;
}

size_t rhashset_unsafe_set(
    rhashset_t *hset, const char *key, size_t size, size_t val) {
  size_t index = rhashset_hash(key, size) % hset->capacity;
  for (;;) {
    rhashset_item_t it = hset->array[index];
    if (it.value == 0) {
      char *key_ptr = (char *)calloc(size, sizeof(char));
      memcpy(key_ptr, key, size);
      hset->array[index] = (rhashset_item_t){
          .value = val,
          .key_size = size,
          .key = key_ptr,
      };
      hset->reverse[val] = key_ptr;
      return hset->size;
    } else if (it.key_size == size && !memcmp(key, it.key, size)) {
      return it.value;
    } else {
      index = (index + 1) % hset->capacity;
    }
  }
}

size_t rhashset_put(rhashset_t *hset, const char *key, size_t size) {
  if (hset->size > hset->capacity * RHASHSET_GROW_RATIO) {
    rhashset_grow(hset, hset->capacity * RHASHSET_GROW_FACTOR);
  }
  return rhashset_unsafe_set(hset, key, size, hset->size++);
}

size_t rhashset_get(rhashset_t *hset, const char *key, size_t size) {
  size_t index = rhashset_hash(key, size) % hset->capacity;
  for (;;) {
    rhashset_item_t it = hset->array[index];
    if (it.value == 0) {
      return 0;
    } else if (it.key_size == size && !memcmp(key, it.key, size)) {
      return it.value;
    } else {
      index = (index + 1) % hset->capacity;
    }
  }
}

const char * rhashset_rev(rhashset_t *hset, size_t key) {
  if (key != 0 || key <= hset->size) {
    return NULL;
  } else {
    return hset->reverse[key];
  }
}
