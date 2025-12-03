#include "block_interface.h"
#include "printer_utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
  char *filename;
  ImageFormat format;
} ImageBlockData;

static void *image_block_parse(json_object *json) {
  ImageBlockData *data = malloc(sizeof(ImageBlockData));
  if (!data)
    return NULL;

  // Defaults - we need to init ImageFormat manually as there isn't a
  // init_default_image_format function exposed or we can use
  // init_default_image_format_from_json from printer_utils.h if we keep it.
  // Let's reimplement for modularity.
  data->format.width = 0;  // Auto
  data->format.height = 0; // Auto
  data->format.x_pos = 0;
  data->format.y_pos = 0;
  data->format.scale = 1;
  data->format.filename[0] = '\0';

  json_object *filename;
  if (json_object_object_get_ex(json, "filename", &filename) ||
      json_object_object_get_ex(json, "src", &filename) ||
      json_object_object_get_ex(json, "path", &filename)) {
    data->filename = strdup(json_object_get_string(filename));
    strncpy(data->format.filename, data->filename,
            sizeof(data->format.filename) - 1);
  } else {
    free(data);
    return NULL;
  }

  json_object *format_obj = NULL;
  json_object *target_obj = json; // Default to root

  if (json_object_object_get_ex(json, "format", &format_obj) &&
      json_object_is_type(format_obj, json_type_object)) {
    target_obj = format_obj;
  }

  json_object *val;
  if (json_object_object_get_ex(target_obj, "width", &val) &&
      json_object_is_type(val, json_type_int)) {
    data->format.width = json_object_get_int(val);
  }
  if (json_object_object_get_ex(target_obj, "height", &val) &&
      json_object_is_type(val, json_type_int)) {
    data->format.height = json_object_get_int(val);
  }
  if (json_object_object_get_ex(target_obj, "x_pos", &val) &&
      json_object_is_type(val, json_type_int)) {
    data->format.x_pos = json_object_get_int(val);
  }
  if (json_object_object_get_ex(target_obj, "y_pos", &val) &&
      json_object_is_type(val, json_type_int)) {
    data->format.y_pos = json_object_get_int(val);
  }
  if (json_object_object_get_ex(target_obj, "scale", &val) &&
      json_object_is_type(val, json_type_int)) {
    data->format.scale = json_object_get_int(val);
  }

  return data;
}

static int image_block_calculate_height(void *ptr, int width) {
  ImageBlockData *data = (ImageBlockData *)ptr;
  if (!data || !data->filename)
    return 0;

  // Load image to get dimensions
  // create_image_surface is in image_utils.c
  cairo_surface_t *image_surface = create_image_surface(
      data->filename, data->format.width, data->format.height);

  if (image_surface) {
    int height = cairo_image_surface_get_height(image_surface);
    if (data->format.scale > 1) {
      height *= data->format.scale;
    }
    cairo_surface_destroy(image_surface);
    return height + 10; // Padding
  }

  return 100; // Fallback
}

static void image_block_render(void *ptr, BlockContext *ctx) {
  ImageBlockData *data = (ImageBlockData *)ptr;
  if (!data || !data->filename)
    return;

  cairo_surface_t *image_surface = create_image_surface(
      data->filename, data->format.width, data->format.height);
  if (!image_surface)
    return;

  int img_w = cairo_image_surface_get_width(image_surface);
  int img_h = cairo_image_surface_get_height(image_surface);

  int scale = data->format.scale;
  if (scale < 1)
    scale = 1;

  int final_w = img_w * scale;
  int final_h = img_h * scale;

  // Calculate X position
  int x = (ctx->width - final_w) / 2; // Default center
  if (data->format.x_pos < 0) {
    x = SIDE_MARGIN;
  } else if (data->format.x_pos > 0) {
    x = ctx->width - final_w - SIDE_MARGIN;
  }

  int y = ctx->current_y;

  cairo_save(ctx->cr);
  cairo_translate(ctx->cr, x, y);
  cairo_scale(ctx->cr, scale, scale);
  cairo_set_source_surface(ctx->cr, image_surface, 0, 0);
  cairo_paint(ctx->cr);
  cairo_restore(ctx->cr);

  cairo_surface_destroy(image_surface);
}

static void image_block_destroy(void *ptr) {
  ImageBlockData *data = (ImageBlockData *)ptr;
  if (data) {
    free(data->filename);
    free(data);
  }
}

const BlockVTable image_block_vtable = {.type_name = "image",
                                        .parse = image_block_parse,
                                        .calculate_height =
                                            image_block_calculate_height,
                                        .render = image_block_render,
                                        .destroy = image_block_destroy};
