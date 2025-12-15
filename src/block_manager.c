#include "block_manager.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Forward declarations of block vtables
extern const BlockVTable text_block_vtable;
extern const BlockVTable qr_block_vtable;
extern const BlockVTable image_block_vtable;
extern const BlockVTable feed_block_vtable;
extern const BlockVTable hr_block_vtable;
extern const BlockVTable table_block_vtable;
extern const BlockVTable cut_block_vtable;

#define MAX_BLOCK_TYPES 10
static const BlockVTable *registry[MAX_BLOCK_TYPES];
static int registry_count = 0;

void block_manager_register(const BlockVTable *vtable) {
  if (registry_count < MAX_BLOCK_TYPES) {
    registry[registry_count++] = vtable;
  }
}

void block_manager_init() {
  registry_count = 0;
  block_manager_register(&text_block_vtable);
  block_manager_register(&qr_block_vtable);
  block_manager_register(&image_block_vtable);
  block_manager_register(&feed_block_vtable);
  block_manager_register(&hr_block_vtable);
  block_manager_register(&table_block_vtable);
  block_manager_register(&cut_block_vtable);
}

static const BlockVTable *find_vtable(const char *type) {
  for (int i = 0; i < registry_count; i++) {
    if (strcmp(registry[i]->type_name, type) == 0) {
      return registry[i];
    }
  }
  return NULL;
}

Block *block_manager_parse(json_object *json_array) {
  if (!json_array || !json_object_is_type(json_array, json_type_array)) {
    return NULL;
  }

  int len = json_object_array_length(json_array);
  Block *blocks = calloc(len + 1, sizeof(Block)); // +1 for terminator

  int block_idx = 0;
  for (int i = 0; i < len; i++) {
    json_object *j_block = json_object_array_get_idx(json_array, i);
    json_object *j_type;

    if (json_object_object_get_ex(j_block, "type", &j_type) &&
        json_object_is_type(j_type, json_type_string)) {

      const char *type_str = json_object_get_string(j_type);
      const BlockVTable *vtable = find_vtable(type_str);

      if (vtable) {
        void *data = vtable->parse(j_block);
        if (data) {
          blocks[block_idx].vtable = vtable;
          blocks[block_idx].data = data;
          block_idx++;
        }
      } else {
        printf("Warning: Unknown block type '%s'\n", type_str);
      }
    }
  }

  return blocks;
}

int block_manager_calculate_height(Block *blocks, int width, int count) {
  if (!blocks)
    return 0;

  int total_height = 0;
  for (int i = 0; (count == -1 ? blocks[i].vtable != NULL : i < count); i++) {
    if (blocks[i].vtable->calculate_height) {
      total_height += blocks[i].vtable->calculate_height(blocks[i].data, width);
    }
  }
  return total_height;
}

void block_manager_render(Block *blocks, cairo_surface_t *surface, int width,
                          int count) {
  if (!blocks || !surface)
    return;

  cairo_t *cr = cairo_create(surface);

  BlockContext ctx;
  ctx.surface = surface;
  ctx.cr = cr;
  ctx.width = width;
  ctx.current_y = 0;

  int current_y = 0;

  for (int i = 0; (count == -1 ? blocks[i].vtable != NULL : i < count); i++) {
    ctx.current_y = current_y;

    if (blocks[i].vtable->render) {
      blocks[i].vtable->render(blocks[i].data, &ctx);
    }

    if (blocks[i].vtable->calculate_height) {
      current_y += blocks[i].vtable->calculate_height(blocks[i].data, width);
    }
  }

  cairo_destroy(cr);
}

void block_list_destroy(Block *blocks) {
  if (!blocks)
    return;

  for (int i = 0; blocks[i].vtable != NULL; i++) {
    if (blocks[i].vtable->destroy) {
      blocks[i].vtable->destroy(blocks[i].data);
    }
  }
  free(blocks);
}
