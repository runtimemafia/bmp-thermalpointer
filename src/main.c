#include "block_manager.h"
#include "printer_utils.h"

int main(int argc, char *argv[]) {
  if (argc < 2) {
    printf("Usage: %s <json_file_or_template> [printer_device_path]\n",
           argv[0]);
    printf("Example 1 (file): %s sample.json /dev/usb/lp0\n", argv[0]);
    printf("Example 2 (inline): %s "
           "'{\"blocks\":[{\"type\":\"text\",\"content\":\"Sample "
           "text\"},{\"type\":\"qr\",\"data\":\"https://"
           "example.com\",\"size\":3}]}' /dev/usb/lp0\n",
           argv[0]);
    return 1;
  }

  const char *json_input = NULL;
  char *json_buffer = NULL;
  const char *printer_device = NULL;

  // Check if first argument is a file
  FILE *json_file = fopen(argv[1], "r");
  if (json_file) {
    // It's a file, read its contents
    fseek(json_file, 0, SEEK_END);
    long file_size = ftell(json_file);
    fseek(json_file, 0, SEEK_SET);

    json_buffer = malloc(file_size + 1);
    if (!json_buffer) {
      printf("Error: Could not allocate memory for JSON file\n");
      fclose(json_file);
      return 1;
    }

    size_t bytes_read = fread(json_buffer, 1, file_size, json_file);
    json_buffer[bytes_read] = '\0';
    fclose(json_file);

    json_input = json_buffer;
    printer_device = (argc > 2) ? argv[2] : NULL;
  } else {
    // It's a JSON string
    json_input = argv[1];
    printer_device = (argc > 2) ? argv[2] : NULL;
  }

  // Initialize Block Manager
  block_manager_init();

  // Parse JSON
  json_object *root = json_tokener_parse(json_input);
  if (!root) {
    printf("Error: Failed to parse JSON input\n");
    if (json_buffer)
      free(json_buffer);
    return 1;
  }

  json_object *blocks_array;
  if (!json_object_object_get_ex(root, "blocks", &blocks_array)) {
    printf("Error: 'blocks' array not found in JSON\n");
    json_object_put(root);
    if (json_buffer)
      free(json_buffer);
    return 1;
  }

  // Determine printer width and global font size
  int printer_width = PRINTER_WIDTH_DOTS;
  json_object *width_obj;
  if (json_object_object_get_ex(root, "width", &width_obj)) {
    if (json_object_is_type(width_obj, json_type_int)) {
      printer_width = json_object_get_int(width_obj);
    }
  }

  json_object *font_size_obj;
  if (json_object_object_get_ex(root, "font_size", &font_size_obj)) {
    if (json_object_is_type(font_size_obj, json_type_int)) {
      set_default_font_size(json_object_get_int(font_size_obj));
    }
  }

  // Parse blocks
  Block *blocks = block_manager_parse(blocks_array);
  if (!blocks) {
    printf("Error: Failed to parse blocks\n");
    json_object_put(root);
    if (json_buffer)
      free(json_buffer);
    return 1;
  }

  // Determine global cut mode
  int global_cut_mode = CUT_MODE_PARTIAL; // Default to partial cut
  json_object *cut_obj;
  if (json_object_object_get_ex(root, "cut", &cut_obj)) {
    const char *cut_str = json_object_get_string(cut_obj);
    if (strcmp(cut_str, "full") == 0) {
      global_cut_mode = CUT_MODE_FULL;
    } else if (strcmp(cut_str, "partial") == 0) {
      global_cut_mode = CUT_MODE_PARTIAL;
    } else if (strcmp(cut_str, "none") == 0) {
      global_cut_mode = CUT_MODE_NONE;
    }
  }

  // Process blocks in segments
  int start_idx = 0;
  int num_blocks = 0;
  // Count total blocks
  for (int i = 0; blocks[i].vtable != NULL; i++) {
    num_blocks++;
  }

  // Open output file if not sending to printer directly, or use a temporary
  // buffer For simplicity, let's accumulate everything into a large buffer or
  // write to a temp file Since we need to send multiple commands to USB
  // printer, it might be fine. Let's use a dynamic buffer.

  unsigned char *final_cmd = NULL;
  int final_cmd_size = 0;

  for (int i = 0; i <= num_blocks; i++) {
    int is_cut_block = 0;
    int segment_cut_mode = CUT_MODE_NONE;

    if (i < num_blocks) {
      if (strcmp(blocks[i].vtable->type_name, "cut") == 0) {
        is_cut_block = 1;
        // Parse cut mode from the block
        // We need to cast data to CutBlockData, but struct definition is
        // internal to cut_block.c We should expose it or add a helper. For now,
        // let's assume we can't access it directly without exposing struct.
        // Let's rely on the fact that we can add a "get_mode" to vtable? No,
        // vtable is generic. Let's just expose the struct in a header or
        // duplicate definition here for now (hacky but fast). Better: add `int
        // (*get_extra_info)(void *data)` to vtable? No. Let's just move
        // CutBlockData to a header. OR, since I am editing cut_block.c, I can
        // make a getter. But I don't want to change vtable interface. I will
        // assume the data is just the int mode for now, or re-parse? Actually,
        // I can just check the JSON again? No, blocks are already parsed.

        // Let's just assume I can cast it. I defined it as:
        // typedef struct { int mode; } CutBlockData;
        // So it's safe to cast to int* if it's the first member.
        segment_cut_mode = *(int *)blocks[i].data;
      }
    } else {
      // End of blocks, apply global cut mode
      segment_cut_mode = global_cut_mode;
    }

    if (is_cut_block || i == num_blocks) {
      int count = i - start_idx;

      if (count > 0) {
        // Calculate height for this segment
        int segment_height = block_manager_calculate_height(
            &blocks[start_idx], printer_width, count);
        segment_height += TOP_PADDING + BOTTOM_PADDING;

        // Render segment
        cairo_surface_t *surface = cairo_image_surface_create(
            CAIRO_FORMAT_ARGB32, printer_width, segment_height);
        cairo_t *cr = cairo_create(surface);
        cairo_set_source_rgb(cr, 1, 1, 1);
        cairo_paint(cr);
        cairo_destroy(cr);

        block_manager_render(&blocks[start_idx], surface, printer_width, count);

        // Convert to bitmap
        int width, height, bmp_size;
        unsigned char *bmp_data =
            convert_to_1bit_bmp(surface, &width, &height, &bmp_size);

        // Create ESC/POS command for this segment
        int cmd_size;
        // We pass CUT_MODE_NONE here because we append the cut command manually
        unsigned char *escpos_cmd = create_escpos_raster_command(
            bmp_data, width, height, CUT_MODE_NONE, &cmd_size);

        // Append to final buffer
        final_cmd = realloc(final_cmd, final_cmd_size + cmd_size);
        memcpy(final_cmd + final_cmd_size, escpos_cmd, cmd_size);
        final_cmd_size += cmd_size;

        free(bmp_data);
        free(escpos_cmd);
        cairo_surface_destroy(surface);
      }

      // Append cut command if needed
      if (segment_cut_mode != CUT_MODE_NONE) {
        unsigned char cut_cmd[4];
        int cut_cmd_len = 0;
        cut_cmd[cut_cmd_len++] = 0x1D;
        cut_cmd[cut_cmd_len++] = 'V';
        cut_cmd[cut_cmd_len++] = (segment_cut_mode == CUT_MODE_FULL) ? 0 : 1;

        final_cmd = realloc(final_cmd, final_cmd_size + cut_cmd_len);
        memcpy(final_cmd + final_cmd_size, cut_cmd, cut_cmd_len);
        final_cmd_size += cut_cmd_len;
      }

      start_idx = i + 1;
    }
  }

  if (printer_device) {
    // Send directly to USB printer
    int result = send_to_usb_printer(final_cmd, final_cmd_size, printer_device);
    if (result == 0) {
      printf("Successfully sent to printer: %s\n", printer_device);
    } else {
      printf("Failed to send to printer: %s. Saving to file instead.\n",
             printer_device);
      save_to_file(final_cmd, final_cmd_size);
    }
  } else {
    // Save to file for testing
    save_to_file(final_cmd, final_cmd_size);
    printf("Printer device not specified. Output saved to output.bin.\n");
  }

  // Save a PNG preview (of the last segment? or maybe we should stitch them?
  // For now, let's just skip PNG preview or render the whole thing for preview)
  // Re-render everything for preview
  int total_height = block_manager_calculate_height(blocks, printer_width, -1);
  cairo_surface_t *preview_surface = cairo_image_surface_create(
      CAIRO_FORMAT_ARGB32, printer_width, total_height);
  cairo_t *cr = cairo_create(preview_surface);
  cairo_set_source_rgb(cr, 1, 1, 1);
  cairo_paint(cr);
  cairo_destroy(cr);
  block_manager_render(blocks, preview_surface, printer_width, -1);
  save_to_png(preview_surface, "preview.png");
  cairo_surface_destroy(preview_surface);

  // Cleanup
  block_list_destroy(blocks);
  json_object_put(root);
  free(final_cmd);
  if (json_buffer)
    free(json_buffer);

  printf("Processed JSON template\n");
  printf("Preview saved to preview.png\n");
  return 0;
}