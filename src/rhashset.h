#ifndef _RHASHSET_HEADER_
#define _RHASHSET_HEADER_

typedef struct rhashset rhashset_t;
typedef struct rhashset_item rhashset_item_t;

rhashset_t *rhashset_create(size_t capacity);
void rhashset_destroy(rhashset_t *hmap);
void rhashset_grow(rhashset_t *hmap, size_t capacity);

size_t rhashset_put(rhashset_t *hmap, const char *key, size_t size);
size_t rhashset_get(rhashset_t *hmap, const char *key, size_t size);
const char * rhashset_rev(rhashset_t *hset, size_t key);

#endif
