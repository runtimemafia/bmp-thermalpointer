#include "block_interface.h"
#include "printer_utils.h"
#include <stdlib.h>
#include <string.h>

typedef struct {
  int mode; // CUT_MODE_FULL or CUT_MODE_PARTIAL
} CutBlockData;

static void *cut_block_parse(json_object *json) {
  CutBlockData *data = malloc(sizeof(CutBlockData));
  data->mode = CUT_MODE_PARTIAL; // Default

  json_object *mode_obj;
  if (json_object_object_get_ex(json, "mode", &mode_obj)) {
    const char *mode_str = json_object_get_string(mode_obj);
    if (strcmp(mode_str, "full") == 0) {
      data->mode = CUT_MODE_FULL;
    } else if (strcmp(mode_str, "partial") == 0) {
      data->mode = CUT_MODE_PARTIAL;
    }
  }
  return data;
}

static int cut_block_calculate_height(void *ptr, int width) {
  (void)ptr;
  (void)width;
  return 0; // Takes no visual space
}

static void cut_block_render(void *ptr, BlockContext *ctx) {
  (void)ptr;
  (void)ctx;
  // No rendering
}

static void cut_block_destroy(void *ptr) { free(ptr); }

const BlockVTable cut_block_vtable = {
    .type_name = "cut",
    .parse = cut_block_parse,
    .calculate_height = cut_block_calculate_height,
    .render = cut_block_render,
    .destroy = cut_block_destroy,
};
