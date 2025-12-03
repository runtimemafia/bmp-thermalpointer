#include "block_interface.h"
#include "printer_utils.h"
#include <pango/pangocairo.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
  char *header;
  char *width_str; // e.g. "50%" or "100"
  char *align;     // "left", "center", "right"
} TableColumn;

typedef struct {
  char *content;
  struct {
    int font_size;        // 0 means inherit
    char font_weight[32]; // empty means inherit
    char align[16];       // empty means inherit
  } format;
} TableCell;

typedef struct {
  TableColumn *columns;
  int column_count;
  TableCell **rows; // Array of rows, each row is array of TableCell
  int row_count;
  struct {
    struct {
      int top;
      int bottom;
      int left;
      int right;
      int inner_horizontal;
      int inner_vertical;
    } borders;
    int cell_padding;
    char header_font_weight[32];
    int font_size;
  } format;
} TableBlockData;

static void *table_block_parse(json_object *json) {
  TableBlockData *data = calloc(1, sizeof(TableBlockData));
  if (!data)
    return NULL;

  // Defaults
  data->format.cell_padding = 5;
  strcpy(data->format.header_font_weight, "bold");
  data->format.font_size = get_default_font_size();

  // Parse columns
  json_object *columns_arr;
  if (json_object_object_get_ex(json, "columns", &columns_arr) &&
      json_object_is_type(columns_arr, json_type_array)) {

    data->column_count = json_object_array_length(columns_arr);
    data->columns = calloc(data->column_count, sizeof(TableColumn));

    for (int i = 0; i < data->column_count; i++) {
      json_object *col_obj = json_object_array_get_idx(columns_arr, i);

      json_object *val;
      if (json_object_object_get_ex(col_obj, "header", &val))
        data->columns[i].header = strdup(json_object_get_string(val));

      if (json_object_object_get_ex(col_obj, "width", &val)) {
        if (json_object_is_type(val, json_type_string))
          data->columns[i].width_str = strdup(json_object_get_string(val));
        else if (json_object_is_type(val, json_type_int)) {
          char buf[32];
          snprintf(buf, sizeof(buf), "%d", json_object_get_int(val));
          data->columns[i].width_str = strdup(buf);
        }
      }

      if (json_object_object_get_ex(col_obj, "align", &val))
        data->columns[i].align = strdup(json_object_get_string(val));
      else
        data->columns[i].align = strdup("left");
    }
  }

  // Parse rows
  json_object *rows_arr;
  if (json_object_object_get_ex(json, "rows", &rows_arr) &&
      json_object_is_type(rows_arr, json_type_array)) {

    data->row_count = json_object_array_length(rows_arr);
    data->rows = calloc(data->row_count, sizeof(TableCell *));

    for (int i = 0; i < data->row_count; i++) {
      json_object *row_arr = json_object_array_get_idx(rows_arr, i);
      if (json_object_is_type(row_arr, json_type_array)) {
        int cols = json_object_array_length(row_arr);
        int safe_cols = (cols < data->column_count) ? cols : data->column_count;

        data->rows[i] = calloc(data->column_count, sizeof(TableCell));
        for (int j = 0; j < safe_cols; j++) {
          json_object *cell_obj = json_object_array_get_idx(row_arr, j);

          if (json_object_is_type(cell_obj, json_type_string)) {
            // Legacy string format
            data->rows[i][j].content = strdup(json_object_get_string(cell_obj));
          } else if (json_object_is_type(cell_obj, json_type_object)) {
            // New object format
            json_object *val;
            if (json_object_object_get_ex(cell_obj, "content", &val)) {
              data->rows[i][j].content = strdup(json_object_get_string(val));
            }

            if (json_object_object_get_ex(cell_obj, "format", &val)) {
              json_object *f_val;
              if (json_object_object_get_ex(val, "font_size", &f_val))
                data->rows[i][j].format.font_size = json_object_get_int(f_val);
              if (json_object_object_get_ex(val, "font_weight", &f_val))
                strncpy(data->rows[i][j].format.font_weight,
                        json_object_get_string(f_val), 31);
              if (json_object_object_get_ex(val, "align", &f_val))
                strncpy(data->rows[i][j].format.align,
                        json_object_get_string(f_val), 15);
            }
          }
        }
      }
    }
  }

  // Parse format
  json_object *format_obj;
  if (json_object_object_get_ex(json, "format", &format_obj)) {
    json_object *val;

    // Handle legacy "border" boolean or new "borders" object
    if (json_object_object_get_ex(format_obj, "borders", &val) &&
        json_object_is_type(val, json_type_object)) {
      json_object *b_val;
      if (json_object_object_get_ex(val, "top", &b_val))
        data->format.borders.top = json_object_get_boolean(b_val);
      if (json_object_object_get_ex(val, "bottom", &b_val))
        data->format.borders.bottom = json_object_get_boolean(b_val);
      if (json_object_object_get_ex(val, "left", &b_val))
        data->format.borders.left = json_object_get_boolean(b_val);
      if (json_object_object_get_ex(val, "right", &b_val))
        data->format.borders.right = json_object_get_boolean(b_val);
      if (json_object_object_get_ex(val, "inner_horizontal", &b_val))
        data->format.borders.inner_horizontal = json_object_get_boolean(b_val);
      if (json_object_object_get_ex(val, "inner_vertical", &b_val))
        data->format.borders.inner_vertical = json_object_get_boolean(b_val);
    } else if (json_object_object_get_ex(format_obj, "border", &val)) {
      int all_borders = json_object_get_boolean(val);
      data->format.borders.top = all_borders;
      data->format.borders.bottom = all_borders;
      data->format.borders.left = all_borders;
      data->format.borders.right = all_borders;
      data->format.borders.inner_horizontal = all_borders;
      data->format.borders.inner_vertical = all_borders;
    }

    if (json_object_object_get_ex(format_obj, "cell_padding", &val))
      data->format.cell_padding = json_object_get_int(val);
    if (json_object_object_get_ex(format_obj, "header_font_weight", &val))
      strncpy(data->format.header_font_weight, json_object_get_string(val), 31);
    if (json_object_object_get_ex(format_obj, "font_size", &val))
      data->format.font_size = json_object_get_int(val);
  }

  return data;
}

static int parse_width(const char *width_str, int total_width) {
  if (!width_str)
    return total_width;
  if (strchr(width_str, '%')) {
    int percent = atoi(width_str);
    return (total_width * percent) / 100;
  } else {
    return atoi(width_str);
  }
}

static int calculate_cell_height(cairo_t *cr, const char *text, int width,
                                 const char *font_weight, int font_size) {
  if (!text)
    return 0;
  PangoLayout *layout = pango_cairo_create_layout(cr);
  char font_string[256];
  snprintf(font_string, sizeof(font_string), "%s %s %d", DEFAULT_FONT_NAME,
           font_weight, font_size);
  PangoFontDescription *font_desc =
      pango_font_description_from_string(font_string);
  pango_layout_set_font_description(layout, font_desc);
  pango_layout_set_text(layout, text, -1);
  pango_layout_set_width(layout, width * PANGO_SCALE);
  pango_layout_set_wrap(layout, PANGO_WRAP_WORD_CHAR);
  int w, h;
  pango_layout_get_pixel_size(layout, &w, &h);
  pango_font_description_free(font_desc);
  g_object_unref(layout);
  return h;
}

static int table_block_calculate_height(void *ptr, int width) {
  TableBlockData *data = (TableBlockData *)ptr;
  if (!data)
    return 0;

  cairo_surface_t *surface =
      cairo_image_surface_create(CAIRO_FORMAT_ARGB32, width, 10);
  cairo_t *cr = cairo_create(surface);

  int total_height = 0;
  int usable_width = width - (2 * SIDE_MARGIN);
  int *col_widths = calloc(data->column_count, sizeof(int));

  for (int i = 0; i < data->column_count; i++) {
    col_widths[i] = parse_width(data->columns[i].width_str, usable_width);
  }

  int max_header_h = 0;
  for (int i = 0; i < data->column_count; i++) {
    if (data->columns[i].header) {
      int h = calculate_cell_height(
          cr, data->columns[i].header,
          col_widths[i] - (2 * data->format.cell_padding),
          data->format.header_font_weight, data->format.font_size);
      if (h > max_header_h)
        max_header_h = h;
    }
  }
  if (max_header_h > 0) {
    total_height += max_header_h + (2 * data->format.cell_padding);
  }

  for (int r = 0; r < data->row_count; r++) {
    int max_row_h = 0;
    if (data->rows[r]) {
      for (int c = 0; c < data->column_count; c++) {
        if (data->rows[r][c].content) {
          // Determine font size and weight for this cell
          int font_size = data->rows[r][c].format.font_size;
          if (font_size == 0)
            font_size = data->format.font_size;

          const char *font_weight = data->rows[r][c].format.font_weight;
          if (font_weight[0] == '\0')
            font_weight = DEFAULT_FONT_WEIGHT;

          int h = calculate_cell_height(cr, data->rows[r][c].content,
                                        col_widths[c] -
                                            (2 * data->format.cell_padding),
                                        font_weight, font_size);
          if (h > max_row_h)
            max_row_h = h;
        }
      }
    }
    if (max_row_h > 0) {
      total_height += max_row_h + (2 * data->format.cell_padding);
    }
  }

  free(col_widths);
  cairo_destroy(cr);
  cairo_surface_destroy(surface);
  return total_height;
}

static void render_cell(cairo_t *cr, const char *text, int x, int y, int w,
                        const char *align, const char *weight, int font_size,
                        int padding) {
  if (!text)
    return;
  PangoLayout *layout = pango_cairo_create_layout(cr);
  char font_string[256];
  snprintf(font_string, sizeof(font_string), "%s %s %d", DEFAULT_FONT_NAME,
           weight, font_size);
  PangoFontDescription *font_desc =
      pango_font_description_from_string(font_string);
  pango_layout_set_font_description(layout, font_desc);
  pango_layout_set_text(layout, text, -1);
  pango_layout_set_width(layout, (w - 2 * padding) * PANGO_SCALE);
  pango_layout_set_wrap(layout, PANGO_WRAP_WORD_CHAR);
  PangoAlignment pango_align = PANGO_ALIGN_LEFT;
  if (strcmp(align, "center") == 0)
    pango_align = PANGO_ALIGN_CENTER;
  else if (strcmp(align, "right") == 0)
    pango_align = PANGO_ALIGN_RIGHT;
  pango_layout_set_alignment(layout, pango_align);
  cairo_set_source_rgb(cr, 0, 0, 0);
  cairo_move_to(cr, x + padding, y + padding);
  pango_cairo_show_layout(cr, layout);
  pango_font_description_free(font_desc);
  g_object_unref(layout);
}

static void draw_line(cairo_t *cr, int x1, int y1, int x2, int y2) {
  cairo_move_to(cr, x1, y1);
  cairo_line_to(cr, x2, y2);
  cairo_stroke(cr);
}

static void table_block_render(void *ptr, BlockContext *ctx) {
  TableBlockData *data = (TableBlockData *)ptr;
  if (!data)
    return;

  int usable_width = ctx->width - (2 * SIDE_MARGIN);
  int *col_widths = calloc(data->column_count, sizeof(int));
  int current_x = SIDE_MARGIN;
  int current_y = ctx->current_y;
  int table_start_y = current_y;

  for (int i = 0; i < data->column_count; i++) {
    col_widths[i] = parse_width(data->columns[i].width_str, usable_width);
  }

  // Render Header
  int max_header_h = 0;
  for (int i = 0; i < data->column_count; i++) {
    if (data->columns[i].header) {
      int h = calculate_cell_height(
          ctx->cr, data->columns[i].header,
          col_widths[i] - (2 * data->format.cell_padding),
          data->format.header_font_weight, data->format.font_size);
      if (h > max_header_h)
        max_header_h = h;
    }
  }

  if (max_header_h > 0) {
    int row_height = max_header_h + (2 * data->format.cell_padding);
    int x = current_x;

    // Top border
    if (data->format.borders.top) {
      draw_line(ctx->cr, SIDE_MARGIN, current_y, SIDE_MARGIN + usable_width,
                current_y);
    }

    for (int i = 0; i < data->column_count; i++) {
      render_cell(ctx->cr, data->columns[i].header, x, current_y, col_widths[i],
                  data->columns[i].align, data->format.header_font_weight,
                  data->format.font_size, data->format.cell_padding);

      // Left border for first column
      if (i == 0 && data->format.borders.left) {
        draw_line(ctx->cr, x, current_y, x, current_y + row_height);
      }
      // Right border for last column
      if (i == data->column_count - 1 && data->format.borders.right) {
        draw_line(ctx->cr, x + col_widths[i], current_y, x + col_widths[i],
                  current_y + row_height);
      }
      // Inner vertical borders
      if (i < data->column_count - 1 && data->format.borders.inner_vertical) {
        draw_line(ctx->cr, x + col_widths[i], current_y, x + col_widths[i],
                  current_y + row_height);
      }

      x += col_widths[i];
    }
    current_y += row_height;

    // Header separator (treat as inner horizontal)
    if (data->format.borders.inner_horizontal) {
      draw_line(ctx->cr, SIDE_MARGIN, current_y, SIDE_MARGIN + usable_width,
                current_y);
    }
  }

  // Render Rows
  for (int r = 0; r < data->row_count; r++) {
    int max_row_h = 0;
    if (data->rows[r]) {
      for (int c = 0; c < data->column_count; c++) {
        if (data->rows[r][c].content) {
          // Determine font size and weight for this cell
          int font_size = data->rows[r][c].format.font_size;
          if (font_size == 0)
            font_size = data->format.font_size;

          const char *font_weight = data->rows[r][c].format.font_weight;
          if (font_weight[0] == '\0')
            font_weight = DEFAULT_FONT_WEIGHT;

          int h = calculate_cell_height(ctx->cr, data->rows[r][c].content,
                                        col_widths[c] -
                                            (2 * data->format.cell_padding),
                                        font_weight, font_size);
          if (h > max_row_h)
            max_row_h = h;
        }
      }
    }

    if (max_row_h > 0) {
      int row_height = max_row_h + (2 * data->format.cell_padding);
      int x = current_x;
      for (int c = 0; c < data->column_count; c++) {
        if (data->rows[r] && data->rows[r][c].content) {
          // Determine font size, weight, and align for this cell
          int font_size = data->rows[r][c].format.font_size;
          if (font_size == 0)
            font_size = data->format.font_size;

          const char *font_weight = data->rows[r][c].format.font_weight;
          if (font_weight[0] == '\0')
            font_weight = DEFAULT_FONT_WEIGHT;

          const char *align = data->rows[r][c].format.align;
          if (align[0] == '\0')
            align = data->columns[c].align;

          render_cell(ctx->cr, data->rows[r][c].content, x, current_y,
                      col_widths[c], align, font_weight, font_size,
                      data->format.cell_padding);
        }

        // Left border
        if (c == 0 && data->format.borders.left) {
          draw_line(ctx->cr, x, current_y, x, current_y + row_height);
        }
        // Right border
        if (c == data->column_count - 1 && data->format.borders.right) {
          draw_line(ctx->cr, x + col_widths[c], current_y, x + col_widths[c],
                    current_y + row_height);
        }
        // Inner vertical
        if (c < data->column_count - 1 && data->format.borders.inner_vertical) {
          draw_line(ctx->cr, x + col_widths[c], current_y, x + col_widths[c],
                    current_y + row_height);
        }

        x += col_widths[c];
      }
      current_y += row_height;

      // Inner horizontal (except last row)
      if (r < data->row_count - 1 && data->format.borders.inner_horizontal) {
        draw_line(ctx->cr, SIDE_MARGIN, current_y, SIDE_MARGIN + usable_width,
                  current_y);
      }
    }
  }

  // Bottom border
  if (data->format.borders.bottom) {
    draw_line(ctx->cr, SIDE_MARGIN, current_y, SIDE_MARGIN + usable_width,
              current_y);
  }

  free(col_widths);
}

static void table_block_destroy(void *ptr) {
  TableBlockData *data = (TableBlockData *)ptr;
  if (!data)
    return;

  if (data->columns) {
    for (int i = 0; i < data->column_count; i++) {
      free(data->columns[i].header);
      free(data->columns[i].width_str);
      free(data->columns[i].align);
    }
    free(data->columns);
  }

  if (data->rows) {
    for (int i = 0; i < data->row_count; i++) {
      if (data->rows[i]) {
        for (int j = 0; j < data->column_count; j++) {
          // Free cell content
          free(data->rows[i][j].content);
        }
        free(data->rows[i]);
      }
    }
    free(data->rows);
  }

  free(data);
}

const BlockVTable table_block_vtable = {.type_name = "table",
                                        .parse = table_block_parse,
                                        .calculate_height =
                                            table_block_calculate_height,
                                        .render = table_block_render,
                                        .destroy = table_block_destroy};
