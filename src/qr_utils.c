#include "printer_utils.h"

// Function to create a QR code Cairo surface
cairo_surface_t* create_qr_surface(const char *data, int scale) {
    // Encode the data into a QR code
    QRcode *qrcode = QRcode_encodeString(data, 0, QR_ECLEVEL_L, QR_MODE_8, 1);
    if (!qrcode) {
        printf("Error: Could not encode QR code\n");
        return NULL;
    }

    int qr_size = qrcode->width;
    int surface_size = qr_size * scale;

    // Create a Cairo surface for the QR code
    cairo_surface_t *qr_surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, surface_size, surface_size);
    cairo_t *qr_cr = cairo_create(qr_surface);

    // Fill background as white
    cairo_set_source_rgb(qr_cr, 1, 1, 1);
    cairo_paint(qr_cr);

    // Set black color for QR code
    cairo_set_source_rgb(qr_cr, 0, 0, 0);

    // Draw the QR code
    for (int y = 0; y < qr_size; y++) {
        for (int x = 0; x < qr_size; x++) {
            if (qrcode->data[y * qr_size + x] & 0x01) {
                // Draw a square for each QR code pixel
                cairo_rectangle(qr_cr, x * scale, y * scale, scale, scale);
                cairo_fill(qr_cr);
            }
        }
    }

    // Cleanup
    QRcode_free(qrcode);
    cairo_destroy(qr_cr);

    return qr_surface;
}

// Enhanced function to render QR with formatting options
void render_qr_to_surface_with_format(cairo_surface_t *surface, const char *qr_data, int base_y, QRFormat *format) {
    cairo_surface_t *qr_surface = create_qr_surface(qr_data, format->size);

    if (qr_surface) {
        // Calculate QR width and position
        int qr_width = cairo_image_surface_get_width(qr_surface);
        int x_pos = 0;

        if (format->x_pos == 0) {
            // Center the QR code horizontally
            x_pos = (PRINTER_WIDTH_DOTS - qr_width) / 2;
        } else if (format->x_pos < 0) {
            // Left aligned with margin
            x_pos = -format->x_pos;  // Use the absolute value as left margin
        } else {
            // Right aligned with margin
            x_pos = PRINTER_WIDTH_DOTS - qr_width - format->x_pos;
        }

        int y_pos = base_y + format->y_pos;

        cairo_t *cr = cairo_create(surface);
        // Draw the QR code onto the main surface
        cairo_set_source_surface(cr, qr_surface, x_pos, y_pos);
        cairo_paint(cr);
        cairo_destroy(cr);

        // Cleanup QR surface
        cairo_surface_destroy(qr_surface);
    }
}

// Basic function to render QR without formatting
void render_qr_to_surface(cairo_surface_t *surface, const char *qr_data, int x, int y, int scale) {
    (void)x; // Suppress unused parameter warning
    cairo_surface_t *qr_surface = create_qr_surface(qr_data, scale);

    if (qr_surface) {
        // Center the QR code horizontally
        int qr_width = cairo_image_surface_get_width(qr_surface);
        int x_pos = (PRINTER_WIDTH_DOTS - qr_width) / 2;

        cairo_t *cr = cairo_create(surface);
        // Draw the QR code onto the main surface
        cairo_set_source_surface(cr, qr_surface, x_pos, y);
        cairo_paint(cr);
        cairo_destroy(cr);

        // Cleanup QR surface
        cairo_surface_destroy(qr_surface);
    }
}