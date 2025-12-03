#include "block_interface.h"
#include <json-c/json.h>
#include <stdlib.h>

typedef struct {
  int lines;
} FeedBlockData;

static void *feed_block_parse(json_object *json) {
  FeedBlockData *data = malloc(sizeof(FeedBlockData));
  if (!data)
    return NULL;

  data->lines = 1; // Default

  json_object *lines;
  if (json_object_object_get_ex(json, "lines", &lines) &&
      json_object_is_type(lines, json_type_int)) {
    data->lines = json_object_get_int(lines);
  }

  return data;
}

static int feed_block_calculate_height(void *ptr, int width) {
  FeedBlockData *data = (FeedBlockData *)ptr;
  if (!data)
    return 0;
  return data->lines * 20; // 20 pixels per line
}

static void feed_block_render(void *ptr, BlockContext *ctx) {
  // Nothing to render, just whitespace
}

static void feed_block_destroy(void *ptr) { free(ptr); }

const BlockVTable feed_block_vtable = {.type_name = "feed",
                                       .parse = feed_block_parse,
                                       .calculate_height =
                                           feed_block_calculate_height,
                                       .render = feed_block_render,
                                       .destroy = feed_block_destroy};
