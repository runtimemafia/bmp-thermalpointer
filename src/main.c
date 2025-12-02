#include "printer_utils.h"

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Usage: %s <json_file_or_template> [printer_device_path]\n", argv[0]);
        printf("Example 1 (file): %s sample.json /dev/usb/lp0\n", argv[0]);
        printf("Example 2 (inline): %s '{\"blocks\":[{\"type\":\"text\",\"content\":\"Sample text\"},{\"type\":\"qr\",\"data\":\"https://example.com\",\"size\":3}]}' /dev/usb/lp0\n", argv[0]);
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

    // Calculate the total height needed for the content
    int total_height = calculate_total_height(json_input);

    // Create a Cairo surface with the calculated height
    cairo_surface_t *surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, PRINTER_WIDTH_DOTS, total_height);
    cairo_t *cr = cairo_create(surface);

    // Fill background as white
    cairo_set_source_rgb(cr, 1, 1, 1);  // White background
    cairo_paint(cr);

    // Render from JSON template
    render_json_blocks(surface, json_input);

    // Convert to 1-bit bitmap
    int width, height, bmp_size;
    unsigned char *bmp_data = convert_to_1bit_bmp(surface, &width, &height, &bmp_size);

    // Create ESC/POS raster command
    int cmd_size;
    unsigned char *escpos_cmd = create_escpos_raster_command(bmp_data, width, height, &cmd_size);

    if (printer_device) {
        // Send directly to USB printer
        int result = send_to_usb_printer(escpos_cmd, cmd_size, printer_device);
        if (result == 0) {
            printf("Successfully sent to printer: %s\n", printer_device);
        } else {
            printf("Failed to send to printer: %s. Saving to file instead.\n", printer_device);
            save_to_file(escpos_cmd, cmd_size);
        }
    } else {
        // Save to file for testing
        save_to_file(escpos_cmd, cmd_size);
        printf("Printer device not specified. Output saved to output.bin.\n");
        printf("To send to printer directly: %s sample.json /dev/usb/lp0\n", argv[0]);
    }

    // Save a PNG preview for visual verification
    save_to_png(surface, "preview.png");

    // Cleanup
    cairo_destroy(cr);
    cairo_surface_destroy(surface);
    free(bmp_data);
    free(escpos_cmd);
    free(json_buffer); // Free the buffer if allocated

    printf("Processed JSON template\n");
    printf("Preview saved to preview.png\n");
    return 0;
}