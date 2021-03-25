#ifndef BBTEX_TAPE_H
#define BBTEX_TAPE_H

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#define TAPE_BUFFER_DEFAULT_SIZE 65536
#define TAPE_DEFAULT_SIZE 4096
#define TAPE_GROW_FACTOR 4

enum tape_key { k_emit = -1, k_update = -2 };
typedef enum tape_key tape_key_t;

struct tape_item {
  tape_key_t key;
  size_t start;
  size_t end;
};
typedef struct tape_item tape_item_t;

struct tape {
  size_t size;
  size_t capacity;
  tape_item_t *array;
};
typedef struct tape tape_t;

tape_t *tape_create(size_t capacity) {
  tape_t *tape = (tape_t *)malloc(sizeof(tape_t));
  tape->size = 0;
  tape->capacity = capacity;
  tape->array = calloc(capacity, sizeof(tape_item_t));
  return tape;
}

void tape_destroy(tape_t *tape) {
  free(tape->array);
  free(tape);
}

void tape_grow(tape_t *tape, size_t capacity) {
  assert(capacity > tape->capacity);
  tape->array = realloc(tape->array, sizeof(tape_item_t) * capacity);
  tape->capacity = capacity;
}

void tape_put(tape_t *tape, tape_key_t key, size_t start, size_t end) {
  if (tape->size >= tape->capacity) {
    tape_grow(tape, tape->capacity * TAPE_GROW_FACTOR);
  }
  tape->array[tape->size++] = (tape_item_t){
      .key = key,
      .start = start,
      .end = end,
  };
}

void tape_put_emit(tape_t *tape) { tape_put(tape, k_emit, 0, 0); }

void tape_put_update(tape_t *tape, size_t update) {
  tape_put(tape, k_update, update, 0);
}

size_t freadx(char *buffer, size_t count, FILE *stream) {
  size_t read = 0;
  size_t total = 0;
  while (
      (read = fread(buffer + total, sizeof(char), (count - total), stream))) {
    total += read;
  }

  return total;
}

size_t freadx_keep(char *buffer, size_t count, size_t keep, FILE *stream) {
  memcpy(buffer + count - keep, buffer, keep);
  size_t read = freadx(buffer + keep, count - keep, stream);
  if (read == 0) {
    return 0;
  } else {
    return read + keep;
  }
}

size_t freadx_start(char *buffer, size_t count, size_t start, FILE *stream) {
  if (start == 0) {
    return freadx(buffer, count, stream);
  } else {
    return freadx_keep(buffer, count, count - start, stream);
  }
}

#endif
