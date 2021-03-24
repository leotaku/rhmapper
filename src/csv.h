#ifndef _BBTEX_CSV_H_
#define _BBTEX_CSV_H_

#include "rhashset.h"
#include "tape.h"

enum csv_state {
  csv_unquoted,
  csv_quoted,
  csv_cr,
};
typedef enum csv_state csv_state_t;

size_t csv_parse_headline(rhashset_t *rh, char b[], FILE *s) {
  size_t total = 0;
  size_t start = 0;
  size_t size = 0;
  while ((size = freadx_start(b, TAPE_BUFFER_DEFAULT_SIZE, start, s))) {
    size_t state = csv_unquoted;
    total += size;
    for (size_t i = 0; i < size; i++) {
      char c = b[i];
      switch (state) {
      case csv_unquoted:
        switch (c) {
        case '"':
          state = csv_quoted;
          break;
        case ',':
          if (b[start] == '"' && b[i - 1] == '"') {
            rhashset_put(rh, b + start + 1, i - start - 2);
          } else {
            rhashset_put(rh, b + start, i - start);
          }
          start = i + 1;
          break;
        case '\r':
          state = csv_cr;
          break;
        }
        break;
      case csv_cr:
        if (c == '\n') {
          if (b[start] == '"' && b[i - 2] == '"') {
            rhashset_put(rh, b + start + 1, i - start - 3);
          } else {
            rhashset_put(rh, b + start, i - start - 1);
          }
          start = i + 1;
          return total;
        } else {
          state = csv_unquoted;
        }
        break;
      case csv_quoted:
        if (c == '"') {
          state = csv_unquoted;
        }
        break;
      }
    }
  }

  // unreachable
  abort();
}

void csv_parse_records(tape_t *tp, size_t offset, char b[], FILE *s) {
  offset = offset % TAPE_BUFFER_DEFAULT_SIZE;
  size_t size = TAPE_BUFFER_DEFAULT_SIZE - offset;
  size_t start = 0;
  size_t key = 1;
  do {
    size_t state = 0;
    for (size_t i = offset; i < size; i++) {
      offset = 0;
      char c = b[i];
      switch (state) {
      case csv_unquoted:
        switch (c) {
        case '"':
          state = csv_quoted;
          break;
        case ',':
          if (b[start] == '"' && b[i - 1] == '"') {
            tape_put(tp, key, start + 1, i - start - 2);
          } else {
            tape_put(tp, key, start, i - start);
          }
          start = i + 1;
          key += 1;
          break;
        case '\r':
          state = csv_cr;
          break;
        }
        break;
      case csv_cr:
        if (c == '\n') {
          if (b[start] == '"' && b[i - 2] == '"') {
            tape_put(tp, key, start + 1, i - start - 3);
          } else {
            tape_put(tp, key, start, i - start - 1);
          }
          tape_put_emit(tp);
          state = csv_unquoted;
          start = i + 1;
        } else {
          state = csv_unquoted;
        }
        break;
      case csv_quoted:
        if (c == '"') {
          state = csv_unquoted;
        }
        break;
      }
    }
  } while ((size = freadx_start(b, size, start, s)));
}

// errno = 116;

void csv_parse(FILE *stream) {
  tape_t *tp = tape_create(TAPE_DEFAULT_SIZE);
  rhashset_t *rh = rhashset_create(256);
  char buffer[TAPE_BUFFER_DEFAULT_SIZE];

  size_t offset = csv_parse_headline(rh, buffer, stream);
  for (size_t i = 0; i < rh->size; i++) {
    puts(rh->reverse[i]);
  }
  csv_parse_records(tp, offset, buffer, stream);
}

#endif
