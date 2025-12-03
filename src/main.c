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

  // Parse blocks
  Block *blocks = block_manager_parse(blocks_array);
  if (!blocks) {
    printf("Error: Failed to parse blocks\n");
    json_object_put(root);
    if (json_buffer)
      free(json_buffer);
    return 1;
  }

  // Calculate total height
  int total_height = block_manager_calculate_height(blocks, PRINTER_WIDTH_DOTS);
  // Add top/bottom padding if needed (previously TOP_PADDING and BOTTOM_PADDING
  // were used)
  total_height += TOP_PADDING + BOTTOM_PADDING;

  // Create Cairo surface
  cairo_surface_t *surface = cairo_image_surface_create(
      CAIRO_FORMAT_ARGB32, PRINTER_WIDTH_DOTS, total_height);
  cairo_t *cr = cairo_create(surface);

  // Fill background as white
  cairo_set_source_rgb(cr, 1, 1, 1);
  cairo_paint(cr);
  cairo_destroy(cr); // We don't need this context anymore, blocks create their
                     // own or use the one passed

  // Render blocks
  // We need to handle the initial offset (TOP_PADDING) if the manager starts at
  // 0 But wait, my block_manager_render starts at 0. I should probably pass the
  // surface to block_manager_render and let it handle it. But I need to respect
  // TOP_PADDING. I can translate the surface or just assume manager handles
  // 0-based and I paste it? Or I can modify block_manager_render to accept a
  // start Y. For now, let's assume I can just render to the surface. If I want
  // TOP_PADDING, I should probably have added a spacer block or handled it in
  // manager. Let's just render.

  // Actually, I can create a sub-surface or just translate the context if I
  // passed a context. But block_manager_render creates its own context. Let's
  // modify block_manager_render to take an offset? Or just rely on the fact
  // that TOP_PADDING is 0 in the header :) "define TOP_PADDING 0"

  block_manager_render(blocks, surface, PRINTER_WIDTH_DOTS);

  // Convert to 1-bit bitmap
  int width, height, bmp_size;
  unsigned char *bmp_data =
      convert_to_1bit_bmp(surface, &width, &height, &bmp_size);

  // Create ESC/POS raster command
  int cmd_size;
  unsigned char *escpos_cmd =
      create_escpos_raster_command(bmp_data, width, height, &cmd_size);

  if (printer_device) {
    // Send directly to USB printer
    int result = send_to_usb_printer(escpos_cmd, cmd_size, printer_device);
    if (result == 0) {
      printf("Successfully sent to printer: %s\n", printer_device);
    } else {
      printf("Failed to send to printer: %s. Saving to file instead.\n",
             printer_device);
      save_to_file(escpos_cmd, cmd_size);
    }
  } else {
    // Save to file for testing
    save_to_file(escpos_cmd, cmd_size);
    printf("Printer device not specified. Output saved to output.bin.\n");
  }

  // Save a PNG preview
  save_to_png(surface, "preview.png");

  // Cleanup
  block_list_destroy(blocks);
  json_object_put(root);
  cairo_surface_destroy(surface);
  free(bmp_data);
  free(escpos_cmd);
  if (json_buffer)
    free(json_buffer);

  printf("Processed JSON template\n");
  printf("Preview saved to preview.png\n");
  return 0;
}