#include "block_interface.h"
#include "printer_utils.h"
#include <qrencode.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
  char *data;
  QRFormat format;
} QRBlockData;

void init_default_qr_format(QRFormat *format) {
  format->size = 3;  // default scale
  format->x_pos = 0; // center
  format->y_pos = 0;
}

static void *qr_block_parse(json_object *json) {
  QRBlockData *data = malloc(sizeof(QRBlockData));
  if (!data)
    return NULL;

  // Defaults
  init_default_qr_format(&data->format);
  data->data = NULL;

  json_object *content;
  if ((json_object_object_get_ex(json, "data", &content) ||
       json_object_object_get_ex(json, "content", &content)) &&
      json_object_is_type(content, json_type_string)) {
    data->data = strdup(json_object_get_string(content));
  } else {
    free(data);
    return NULL;
  }

  // Overrides
  json_object *format_obj = NULL;
  json_object *target_obj = json; // Default to root

  if (json_object_object_get_ex(json, "format", &format_obj) &&
      json_object_is_type(format_obj, json_type_object)) {
    target_obj = format_obj;
  }

  json_object *val;
  if (json_object_object_get_ex(target_obj, "size", &val) &&
      json_object_is_type(val, json_type_int)) {
    data->format.size = json_object_get_int(val);
  }
  if (json_object_object_get_ex(target_obj, "x_pos", &val) &&
      json_object_is_type(val, json_type_int)) {
    data->format.x_pos = json_object_get_int(val);
  }
  if (json_object_object_get_ex(target_obj, "y_pos", &val) &&
      json_object_is_type(val, json_type_int)) {
    data->format.y_pos = json_object_get_int(val);
  }

  return data;
}

static int qr_block_calculate_height(void *ptr, int width) {
  QRBlockData *data = (QRBlockData *)ptr;
  if (!data || !data->data)
    return 0;

  QRcode *qrcode =
      QRcode_encodeString(data->data, 0, QR_ECLEVEL_L, QR_MODE_8, 1);
  int qr_height = 0;
  if (qrcode) {
    int qr_size = qrcode->width;
    qr_height = qr_size * data->format.size;
    QRcode_free(qrcode);
  } else {
    qr_height = 30 * data->format.size; // Fallback
  }

  return qr_height + 10; // Padding
}

static void qr_block_render(void *ptr, BlockContext *ctx) {
  QRBlockData *data = (QRBlockData *)ptr;
  if (!data || !data->data)
    return;

  QRcode *qrcode =
      QRcode_encodeString(data->data, 0, QR_ECLEVEL_L, QR_MODE_8, 1);
  if (!qrcode)
    return;

  int qr_size = qrcode->width;
  int scale = data->format.size;
  int pixel_size = qr_size * scale;

  // Calculate X position
  int x = (ctx->width - pixel_size) / 2; // Default center
  if (data->format.x_pos < 0) {
    x = SIDE_MARGIN; // Left
  } else if (data->format.x_pos > 0) {
    x = ctx->width - pixel_size - SIDE_MARGIN; // Right
  }

  int y = ctx->current_y;

  cairo_set_source_rgb(ctx->cr, 0, 0, 0);

  for (int i = 0; i < qr_size; i++) {
    for (int j = 0; j < qr_size; j++) {
      if (qrcode->data[i * qr_size + j] & 1) {
        cairo_rectangle(ctx->cr, x + (j * scale), y + (i * scale), scale,
                        scale);
        cairo_fill(ctx->cr);
      }
    }
  }

  QRcode_free(qrcode);
}

static void qr_block_destroy(void *ptr) {
  QRBlockData *data = (QRBlockData *)ptr;
  if (data) {
    free(data->data);
    free(data);
  }
}

const BlockVTable qr_block_vtable = {.type_name = "qr",
                                     .parse = qr_block_parse,
                                     .calculate_height =
                                         qr_block_calculate_height,
                                     .render = qr_block_render,
                                     .destroy = qr_block_destroy};
