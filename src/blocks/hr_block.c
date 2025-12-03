#include "block_interface.h"
#include "printer_utils.h" // For SIDE_MARGIN
#include <stdlib.h>

static void *hr_block_parse(json_object *json) {
  // No data needed for HR
  return malloc(1); // Dummy allocation
}

static int hr_block_calculate_height(void *ptr, int width) {
  return 15; // Fixed height
}

static void hr_block_render(void *ptr, BlockContext *ctx) {
  cairo_set_source_rgb(ctx->cr, 0, 0, 0);
  cairo_set_line_width(ctx->cr, 1);
  cairo_move_to(ctx->cr, SIDE_MARGIN, ctx->current_y + 5);
  cairo_line_to(ctx->cr, ctx->width - SIDE_MARGIN, ctx->current_y + 5);
  cairo_stroke(ctx->cr);
}

static void hr_block_destroy(void *ptr) { free(ptr); }

const BlockVTable hr_block_vtable = {.type_name = "hr",
                                     .parse = hr_block_parse,
                                     .calculate_height =
                                         hr_block_calculate_height,
                                     .render = hr_block_render,
                                     .destroy = hr_block_destroy};
