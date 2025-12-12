#include "block_interface.h"
#include "printer_utils.h" // For constants and TextFormat (if we keep it there) or we define it here
#include <pango/pangocairo.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// We can either use the struct from printer_utils.h or define a private one.
// Since we are refactoring, let's define a private one to decouple,
// but for now reusing the existing one in printer_utils.h is safer for
// compatibility if other parts use it. However, the goal is modularity. Let's
// define a private struct for the block data.

typedef struct {
  char *content;
  TextFormat format;
} TextBlockData;

static void *text_block_parse(json_object *json) {
  TextBlockData *data = malloc(sizeof(TextBlockData));
  if (!data)
    return NULL;

  // Defaults
  init_default_text_format(&data->format);
  data->content = NULL;

  // Parse content
  json_object *content;
  if (json_object_object_get_ex(json, "content", &content) &&
      json_object_is_type(content, json_type_string)) {
    data->content = strdup(json_object_get_string(content));
  } else {
    free(data);
    return NULL;
  }

  // Parse format overrides
  json_object *format_obj = NULL;
  json_object *target_obj = json; // Default to root

  if (json_object_object_get_ex(json, "format", &format_obj) &&
      json_object_is_type(format_obj, json_type_object)) {
    target_obj = format_obj;
  }

  json_object *val;
  if (json_object_object_get_ex(target_obj, "font_size", &val) &&
      json_object_is_type(val, json_type_int)) {
    data->format.font_size = json_object_get_int(val);
  }
  if (json_object_object_get_ex(target_obj, "font_weight", &val) &&
      json_object_is_type(val, json_type_string)) {
    strncpy(data->format.font_weight, json_object_get_string(val),
            sizeof(data->format.font_weight) - 1);
  }
  if (json_object_object_get_ex(target_obj, "alignment", &val) &&
      json_object_is_type(val, json_type_string)) {
    strncpy(data->format.alignment, json_object_get_string(val),
            sizeof(data->format.alignment) - 1);
  }
  if (json_object_object_get_ex(target_obj, "top_padding", &val) &&
      json_object_is_type(val, json_type_int)) {
    data->format.top_padding = json_object_get_int(val);
  }
  if (json_object_object_get_ex(target_obj, "bottom_padding", &val) &&
      json_object_is_type(val, json_type_int)) {
    data->format.bottom_padding = json_object_get_int(val);
  }
  if (json_object_object_get_ex(target_obj, "left_padding", &val) &&
      json_object_is_type(val, json_type_int)) {
    data->format.left_padding = json_object_get_int(val);
  }
  if (json_object_object_get_ex(target_obj, "right_padding", &val) &&
      json_object_is_type(val, json_type_int)) {
    data->format.right_padding = json_object_get_int(val);
  }
  if (json_object_object_get_ex(target_obj, "line_spacing", &val)) {
    if (json_object_is_type(val, json_type_double)) {
      data->format.line_spacing = (float)json_object_get_double(val);
    } else if (json_object_is_type(val, json_type_int)) {
      data->format.line_spacing = (float)json_object_get_int(val);
    }
  }

  return data;
}

static int text_block_calculate_height(void *ptr, int width) {
  TextBlockData *data = (TextBlockData *)ptr;
  if (!data || !data->content)
    return 0;

  // We need a dummy surface/context to calculate Pango layout
  cairo_surface_t *dummy_surface =
      cairo_image_surface_create(CAIRO_FORMAT_ARGB32, width, 10);
  cairo_t *cr = cairo_create(dummy_surface);

  PangoLayout *layout = pango_cairo_create_layout(cr);

  char font_string[256];
  snprintf(font_string, sizeof(font_string), "%s %s %d", DEFAULT_FONT_NAME,
           data->format.font_weight, data->format.font_size);
  PangoFontDescription *font_desc =
      pango_font_description_from_string(font_string);

  pango_layout_set_font_description(layout, font_desc);
  pango_layout_set_text(layout, data->content, -1);
  if (data->format.line_spacing > 0.0) {
    pango_layout_set_line_spacing(layout, data->format.line_spacing);
  }

  int text_width = width - data->format.left_padding -
                   data->format.right_padding - (2 * SIDE_MARGIN);
  pango_layout_set_width(layout, text_width * PANGO_SCALE);
  pango_layout_set_wrap(layout, PANGO_WRAP_WORD_CHAR);

  int layout_width, layout_height;
  pango_layout_get_pixel_size(layout, &layout_width, &layout_height);

  int total_height =
      layout_height + data->format.top_padding + data->format.bottom_padding;

  // Add the default padding that was in json_utils.c
  // "current_y += text_height + 10;" or "extra_padding = (format.bottom_padding
  // == 0) ? 10 : 0;" Let's stick to the explicit padding in the struct + maybe
  // a default if 0? The original code had: `int extra_padding =
  // (format.bottom_padding == 0) ? 10 : 0;`
  if (data->format.bottom_padding == 0) {
    total_height += 10;
  }

  pango_font_description_free(font_desc);
  g_object_unref(layout);
  cairo_destroy(cr);
  cairo_surface_destroy(dummy_surface);

  return total_height;
}

static void text_block_render(void *ptr, BlockContext *ctx) {
  TextBlockData *data = (TextBlockData *)ptr;
  if (!data || !data->content)
    return;

  PangoLayout *layout = pango_cairo_create_layout(ctx->cr);

  char font_string[256];
  snprintf(font_string, sizeof(font_string), "%s %s %d", DEFAULT_FONT_NAME,
           data->format.font_weight, data->format.font_size);
  PangoFontDescription *font_desc =
      pango_font_description_from_string(font_string);

  pango_layout_set_font_description(layout, font_desc);
  pango_layout_set_text(layout, data->content, -1);
  if (data->format.line_spacing > 0.0) {
    pango_layout_set_line_spacing(layout, data->format.line_spacing);
  }

  int text_width = ctx->width - data->format.left_padding -
                   data->format.right_padding - (2 * SIDE_MARGIN);
  pango_layout_set_width(layout, text_width * PANGO_SCALE);
  pango_layout_set_wrap(layout, PANGO_WRAP_WORD_CHAR);

  PangoAlignment alignment = PANGO_ALIGN_LEFT;
  if (strcmp(data->format.alignment, "center") == 0) {
    alignment = PANGO_ALIGN_CENTER;
  } else if (strcmp(data->format.alignment, "right") == 0) {
    alignment = PANGO_ALIGN_RIGHT;
  }
  pango_layout_set_alignment(layout, alignment);

  cairo_set_source_rgb(ctx->cr, 0, 0, 0);

  int pos_x = SIDE_MARGIN + data->format.left_padding;
  int pos_y = ctx->current_y + data->format.top_padding;

  cairo_move_to(ctx->cr, pos_x, pos_y);
  pango_cairo_show_layout(ctx->cr, layout);

  pango_font_description_free(font_desc);
  g_object_unref(layout);
}

static void text_block_destroy(void *ptr) {
  TextBlockData *data = (TextBlockData *)ptr;
  if (data) {
    free(data->content);
    free(data);
  }
}

const BlockVTable text_block_vtable = {.type_name = "text",
                                       .parse = text_block_parse,
                                       .calculate_height =
                                           text_block_calculate_height,
                                       .render = text_block_render,
                                       .destroy = text_block_destroy};
