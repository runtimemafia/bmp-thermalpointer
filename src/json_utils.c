#include "printer_utils.h"

// Function to calculate image height based on file and format
int calculate_image_height(const char *filename, ImageFormat *format) {
    // Load image to get its dimensions
    cairo_surface_t *image_surface = create_image_surface(filename, format->width, format->height);
    
    if (image_surface) {
        int height = cairo_image_surface_get_height(image_surface);
        if (format->scale > 1) {
            height *= format->scale;
        }
        cairo_surface_destroy(image_surface);
        return height + 10; // Add padding after image
    }
    
    // Fallback if image can't be loaded
    return 100; // Default height
}

// Function to calculate total height needed for all blocks
int calculate_total_height(const char *json_str) {
    json_object *json = json_tokener_parse(json_str);
    if (!json) {
        printf("Error parsing JSON in height calculation\n");
        return 800; // fallback height
    }

    json_object *blocks;
    if (!json_object_object_get_ex(json, "blocks", &blocks)) {
        printf("Error: 'blocks' field not found in JSON\n");
        json_object_put(json);
        return 800; // fallback height
    }

    if (!json_object_is_type(blocks, json_type_array)) {
        printf("Error: 'blocks' is not an array\n");
        json_object_put(json);
        return 800; // fallback height
    }

    int current_y = TOP_PADDING; // Start from top padding

    int array_length = json_object_array_length(blocks);
    for (int i = 0; i < array_length; i++) {
        json_object *block = json_object_array_get_idx(blocks, i);

        json_object *type;
        if (!json_object_object_get_ex(block, "type", &type) ||
            !json_object_is_type(type, json_type_string)) {
            printf("Error: Block type is not a string\n");
            continue;
        }

        const char *type_str = json_object_get_string(type);

        if (strcmp(type_str, "text") == 0) {
            json_object *content;
            if (json_object_object_get_ex(block, "content", &content) &&
                json_object_is_type(content, json_type_string)) {
                const char *content_str = json_object_get_string(content);

                // Initialize text format from JSON
                TextFormat format;
                init_default_text_format_from_json(block, &format);

                // Calculate text height with format
                int text_width = PRINTER_WIDTH_DOTS - (2 * SIDE_MARGIN);
                int text_height = calculate_text_height_with_format(content_str, text_width, &format);

                // Calculate padding based on format
                int extra_padding = (format.bottom_padding == 0) ? 10 : 0;
                current_y += text_height + extra_padding;
            }
        }
        else if (strcmp(type_str, "qr") == 0) {
            json_object *qr_data;

            if (json_object_object_get_ex(block, "data", &qr_data) &&
                json_object_is_type(qr_data, json_type_string)) {
                // Initialize QR format from JSON
                QRFormat format;
                init_default_qr_format_from_json(block, &format);

                // Calculate the QR code height based on actual QR dimensions
                // First, we need to create the QR code to get its actual size
                const char *qr_data_str = json_object_get_string(qr_data);
                QRcode *qrcode = QRcode_encodeString(qr_data_str, 0, QR_ECLEVEL_L, QR_MODE_8, 1);
                if (qrcode) {
                    int qr_size = qrcode->width;
                    int qr_height = qr_size * format.size; // Apply the scale factor
                    QRcode_free(qrcode); // Free immediately after getting dimensions
                    current_y += qr_height + 10; // Add some padding after QR
                } else {
                    // Fallback if QR code creation fails
                    int qr_approx_width = 30 * format.size; // Based on scale
                    current_y += qr_approx_width + 10;
                }
            }
        }
        else if (strcmp(type_str, "image") == 0) {
            json_object *img_filename;

            if (json_object_object_get_ex(block, "filename", &img_filename) ||
                json_object_object_get_ex(block, "src", &img_filename)) {
                // Initialize image format from JSON
                ImageFormat format;
                init_default_image_format_from_json(block, &format);

                // Calculate image height with format
                const char *filename_str = json_object_get_string(img_filename);
                int img_height = calculate_image_height(filename_str, &format);

                current_y += img_height;
            }
        }
        else if (strcmp(type_str, "hr") == 0) {
            current_y += 15; // Add space for horizontal rule
        }
        else if (strcmp(type_str, "feed") == 0) {
            json_object *lines;
            if (json_object_object_get_ex(block, "lines", &lines) &&
                json_object_is_type(lines, json_type_int)) {
                // For now, just add space equivalent to the number of lines
                current_y += json_object_get_int(lines) * 20; // Approximate pixels per line
            } else {
                current_y += 20; // Default feed of one line
            }
        }
    }

    json_object_put(json);

    // Add some bottom padding
    return current_y + BOTTOM_PADDING;
}

// Function to render the blocks from JSON template
void render_json_blocks(cairo_surface_t *surface, const char *json_str) {
    json_object *json = json_tokener_parse(json_str);
    if (!json) {
        printf("Error parsing JSON\n");
        return;
    }

    json_object *blocks;
    if (!json_object_object_get_ex(json, "blocks", &blocks)) {
        printf("Error: 'blocks' field not found in JSON\n");
        json_object_put(json);
        return;
    }

    if (!json_object_is_type(blocks, json_type_array)) {
        printf("Error: 'blocks' is not an array\n");
        json_object_put(json);
        return;
    }

    int current_y = TOP_PADDING; // Start from top padding

    int array_length = json_object_array_length(blocks);
    for (int i = 0; i < array_length; i++) {
        json_object *block = json_object_array_get_idx(blocks, i);

        json_object *type;
        if (!json_object_object_get_ex(block, "type", &type) ||
            !json_object_is_type(type, json_type_string)) {
            printf("Error: Block type is not a string\n");
            continue;
        }

        const char *type_str = json_object_get_string(type);

        if (strcmp(type_str, "text") == 0) {
            json_object *content;
            if (json_object_object_get_ex(block, "content", &content) &&
                json_object_is_type(content, json_type_string)) {
                const char *content_str = json_object_get_string(content);

                // Initialize text format from JSON
                TextFormat format;
                init_default_text_format_from_json(block, &format);

                // Calculate text height with format
                int text_width = PRINTER_WIDTH_DOTS - (2 * SIDE_MARGIN);
                int text_height = calculate_text_height_with_format(content_str, text_width, &format);

                // Render text with format
                render_text_to_surface_with_format(surface, content_str, SIDE_MARGIN, current_y, &format);
                current_y += text_height + 10; // Add some padding after text
            }
        }
        else if (strcmp(type_str, "qr") == 0) {
            json_object *qr_data;

            if (json_object_object_get_ex(block, "data", &qr_data) &&
                json_object_is_type(qr_data, json_type_string)) {
                const char *qr_data_str = json_object_get_string(qr_data);

                // Initialize QR format from JSON
                QRFormat format;
                init_default_qr_format_from_json(block, &format);

                // Calculate the QR code height based on actual QR dimensions for positioning
                // We'll do the same calculation as in calculate_total_height to maintain consistency
                QRcode *qrcode = QRcode_encodeString(qr_data_str, 0, QR_ECLEVEL_L, QR_MODE_8, 1);
                int qr_height;
                if (qrcode) {
                    int qr_size = qrcode->width;
                    qr_height = qr_size * format.size; // Apply the scale factor
                    QRcode_free(qrcode); // Free immediately after getting dimensions
                } else {
                    // Fallback if QR code creation fails
                    qr_height = 30 * format.size; // Based on scale
                }

                // Render QR with format
                render_qr_to_surface_with_format(surface, qr_data_str, current_y, &format);

                // Use the calculated height to advance current_y
                current_y += qr_height + 10; // Add some padding after QR
            }
        }
        else if (strcmp(type_str, "image") == 0) {
            json_object *img_filename;

            if (json_object_object_get_ex(block, "filename", &img_filename) ||
                json_object_object_get_ex(block, "src", &img_filename)) {
                const char *filename_str = json_object_get_string(img_filename);

                // Initialize image format from JSON
                ImageFormat format;
                init_default_image_format_from_json(block, &format);

                // Render image with format
                render_image_to_surface_with_format(surface, current_y, &format);

                // Calculate image height for positioning next element
                int img_height = calculate_image_height(filename_str, &format);
                current_y += img_height;
            }
        }
        else if (strcmp(type_str, "hr") == 0) {
            // Horizontal rule
            cairo_t *cr = cairo_create(surface);
            cairo_set_source_rgb(cr, 0, 0, 0); // Black
            cairo_set_line_width(cr, 1);
            cairo_move_to(cr, SIDE_MARGIN, current_y + 5);
            cairo_line_to(cr, PRINTER_WIDTH_DOTS - SIDE_MARGIN, current_y + 5);
            cairo_stroke(cr);
            cairo_destroy(cr);
            current_y += 15; // Add space after horizontal rule
        }
        else if (strcmp(type_str, "feed") == 0) {
            json_object *lines;
            if (json_object_object_get_ex(block, "lines", &lines) &&
                json_object_is_type(lines, json_type_int)) {
                // For now, just add space equivalent to the number of lines
                current_y += json_object_get_int(lines) * 20; // Approximate pixels per line
            } else {
                current_y += 20; // Default feed of one line
            }
        }
    }

    json_object_put(json);
}