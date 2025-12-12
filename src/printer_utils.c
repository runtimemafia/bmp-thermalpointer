#include "printer_utils.h"

static int g_default_font_size = DEFAULT_FONT_SIZE;

void set_default_font_size(int size) {
  if (size > 0) {
    g_default_font_size = size;
  }
}

int get_default_font_size() { return g_default_font_size; }

void init_default_text_format(TextFormat *format) {
  format->font_size = g_default_font_size;
  strcpy(format->font_weight, DEFAULT_FONT_WEIGHT);
  strcpy(format->alignment, "left");
  format->top_padding = 0;
  format->bottom_padding = 0;
  format->left_padding = 0;
  format->right_padding = 0;
  format->line_spacing = 0.0;
}
