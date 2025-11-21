#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <cairo.h>
#include <pango/pango.h>
#include <pango/pangocairo.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/usbdevice_fs.h>
#include <qrencode.h>
#include <json-c/json.h>

// Font configuration
#define DEFAULT_FONT_NAME "Noto Sans Malayalam"
#define DEFAULT_FONT_SIZE 24
#define DEFAULT_FONT_WEIGHT "Light"  // Change to "Bold", "Thin", "Light", etc. as needed

// Printer configuration
#define PRINTER_WIDTH_MM 48  // Effective printing width from KP628E manual (48mm)
#define PRINTER_WIDTH_DOTS 384  // Standard thermal printer width in dots (48mm * 8 dots/mm = 384 dots)
#define DPI 203              // Dots per inch from KP628E manual (8 dots/mm = 203 DPI)

// Padding configuration
#define TOP_PADDING 0       // Top padding in pixels
#define BOTTOM_PADDING 0    // Bottom padding in pixels
#define SIDE_MARGIN 10       // Side margin in pixels

// ESC/POS Commands
#define ESC_CMD '@'          // Initialize printer
#define GS_CMD 'v'           // Raster bitmap command
#define GS_SUBCMD '0'        // Raster format 0
#define ESC_CMD_FEED 'd'     // Feed n lines
#define GS_CMD_CUT 'V'       // Paper cut

// Structure to hold text formatting options
typedef struct {
    int font_size;
    char font_weight[32];
    char alignment[16];  // "left", "center", "right"
    int top_padding;
    int bottom_padding;
    int left_padding;
    int right_padding;
} TextFormat;

// Structure to hold QR formatting options
typedef struct {
    int size;
    int x_pos;  // 0=center, negative=left, positive=right margin
    int y_pos;
} QRFormat;

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

// Function to initialize default text format
void init_default_text_format(TextFormat *format) {
    format->font_size = DEFAULT_FONT_SIZE;
    strcpy(format->font_weight, DEFAULT_FONT_WEIGHT);
    strcpy(format->alignment, "left");
    format->top_padding = 0;
    format->bottom_padding = 0;
    format->left_padding = 0;
    format->right_padding = 0;
}

// Function to initialize default QR format
void init_default_qr_format(QRFormat *format) {
    format->size = 3;  // default scale
    format->x_pos = 0; // center
    format->y_pos = 0;
}

// Function to initialize default text format
void init_default_text_format_from_json(json_object *block, TextFormat *format) {
    init_default_text_format(format);
    
    // Override defaults with JSON values if present
    json_object *font_size, *font_weight, *alignment, *top_padding, *bottom_padding, *left_padding, *right_padding;
    
    if (json_object_object_get_ex(block, "font_size", &font_size) && json_object_is_type(font_size, json_type_int)) {
        format->font_size = json_object_get_int(font_size);
    }
    
    if (json_object_object_get_ex(block, "font_weight", &font_weight) && json_object_is_type(font_weight, json_type_string)) {
        const char *weight_str = json_object_get_string(font_weight);
        strncpy(format->font_weight, weight_str, sizeof(format->font_weight) - 1);
        format->font_weight[sizeof(format->font_weight) - 1] = '\0';
    }
    
    if (json_object_object_get_ex(block, "alignment", &alignment) && json_object_is_type(alignment, json_type_string)) {
        const char *align_str = json_object_get_string(alignment);
        strncpy(format->alignment, align_str, sizeof(format->alignment) - 1);
        format->alignment[sizeof(format->alignment) - 1] = '\0';
    }
    
    if (json_object_object_get_ex(block, "top_padding", &top_padding) && json_object_is_type(top_padding, json_type_int)) {
        format->top_padding = json_object_get_int(top_padding);
    }
    
    if (json_object_object_get_ex(block, "bottom_padding", &bottom_padding) && json_object_is_type(bottom_padding, json_type_int)) {
        format->bottom_padding = json_object_get_int(bottom_padding);
    }
    
    if (json_object_object_get_ex(block, "left_padding", &left_padding) && json_object_is_type(left_padding, json_type_int)) {
        format->left_padding = json_object_get_int(left_padding);
    }
    
    if (json_object_object_get_ex(block, "right_padding", &right_padding) && json_object_is_type(right_padding, json_type_int)) {
        format->right_padding = json_object_get_int(right_padding);
    }
}

// Function to initialize default QR format from JSON
void init_default_qr_format_from_json(json_object *block, QRFormat *format) {
    init_default_qr_format(format);
    
    // Override defaults with JSON values if present
    json_object *size, *x_pos, *y_pos;
    
    if (json_object_object_get_ex(block, "size", &size) && json_object_is_type(size, json_type_int)) {
        format->size = json_object_get_int(size);
    }
    
    if (json_object_object_get_ex(block, "x_pos", &x_pos) && json_object_is_type(x_pos, json_type_int)) {
        format->x_pos = json_object_get_int(x_pos);
    }
    
    if (json_object_object_get_ex(block, "y_pos", &y_pos) && json_object_is_type(y_pos, json_type_int)) {
        format->y_pos = json_object_get_int(y_pos);
    }
}

// Function to calculate the height required for the text with custom format
int calculate_text_height_with_format(const char *text, int width, TextFormat *format) {
    cairo_surface_t *dummy_surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, width, 100);
    cairo_t *cr = cairo_create(dummy_surface);

    PangoLayout *layout = pango_cairo_create_layout(cr);

    // Create font string with configurable weight and size from format
    char font_string[256];
    snprintf(font_string, sizeof(font_string), "%s %s %d", DEFAULT_FONT_NAME, format->font_weight, format->font_size);
    PangoFontDescription *font_desc = pango_font_description_from_string(font_string);

    pango_layout_set_font_description(layout, font_desc);
    pango_layout_set_text(layout, text, -1);
    
    // Calculate available width for text (full width minus margins and padding)
    int text_width = width - format->left_padding - format->right_padding;
    pango_layout_set_width(layout, text_width * PANGO_SCALE);
    pango_layout_set_wrap(layout, PANGO_WRAP_WORD_CHAR);

    int layout_width, layout_height;
    pango_layout_get_pixel_size(layout, &layout_width, &layout_height);

    // Calculate total height including padding
    int total_height = layout_height + format->top_padding + format->bottom_padding;

    // Cleanup
    pango_font_description_free(font_desc);
    g_object_unref(layout);
    cairo_destroy(cr);
    cairo_surface_destroy(dummy_surface);

    return total_height;
}

// Enhanced function to render text with formatting options
void render_text_to_surface_with_format(cairo_surface_t *surface, const char *text, int base_x, int base_y, TextFormat *format) {
    cairo_t *cr = cairo_create(surface);

    // Set up Pango for text rendering
    PangoLayout *layout = pango_cairo_create_layout(cr);

    // Create font string with configurable weight and size from format
    char font_string[256];
    snprintf(font_string, sizeof(font_string), "%s %s %d", DEFAULT_FONT_NAME, format->font_weight, format->font_size);
    PangoFontDescription *font_desc = pango_font_description_from_string(font_string);

    pango_layout_set_font_description(layout, font_desc);
    pango_layout_set_text(layout, text, -1);

    // Calculate available width for text (full width minus margins and padding)
    int text_width = PRINTER_WIDTH_DOTS - (2 * base_x) - format->left_padding - format->right_padding;
    pango_layout_set_width(layout, text_width * PANGO_SCALE);
    pango_layout_set_wrap(layout, PANGO_WRAP_WORD_CHAR);

    // Set alignment
    PangoAlignment alignment = PANGO_ALIGN_LEFT;
    if (strcmp(format->alignment, "center") == 0) {
        alignment = PANGO_ALIGN_CENTER;
    } else if (strcmp(format->alignment, "right") == 0) {
        alignment = PANGO_ALIGN_RIGHT;
    }
    pango_layout_set_alignment(layout, alignment);

    // Set color to black
    cairo_set_source_rgb(cr, 0, 0, 0);

    // Calculate text position with alignment and padding
    int pos_x = base_x + format->left_padding;
    if (strcmp(format->alignment, "center") == 0) {
        int layout_width, layout_height;
        pango_layout_get_pixel_size(layout, &layout_width, &layout_height);
        pos_x = (PRINTER_WIDTH_DOTS - layout_width) / 2;
    } else if (strcmp(format->alignment, "right") == 0) {
        int layout_width, layout_height;
        pango_layout_get_pixel_size(layout, &layout_width, &layout_height);
        pos_x = PRINTER_WIDTH_DOTS - layout_width - base_x - format->right_padding;
    }

    // Move to position with top padding
    cairo_move_to(cr, pos_x, base_y + format->top_padding);

    // Render the layout
    pango_cairo_show_layout(cr, layout);

    // Cleanup
    pango_font_description_free(font_desc);
    g_object_unref(layout);
    cairo_destroy(cr);
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

// Function declarations
int calculate_text_height(const char *text, int width);
void render_text_to_surface(cairo_surface_t *surface, const char *text, int x, int y);
void render_qr_to_surface(cairo_surface_t *surface, const char *qr_data, int x, int y, int scale);
unsigned char* convert_to_1bit_bmp(cairo_surface_t *surface, int *width, int *height, int *size);
unsigned char* create_escpos_raster_command(unsigned char *bitmap_data, int width, int height, int *cmd_size);
int send_to_usb_printer(unsigned char *data, int size, const char *device_path);
void save_to_file(unsigned char *data, int size);
void save_to_png(cairo_surface_t *surface, const char *filename);
int calculate_combined_height(const char *text, int text_width, int qr_width);
void render_json_blocks(cairo_surface_t *surface, const char *json_str);

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
                
                current_y += text_height + 10; // Add some padding after text
            }
        }
        else if (strcmp(type_str, "qr") == 0) {
            json_object *qr_data;
            
            if (json_object_object_get_ex(block, "data", &qr_data) && 
                json_object_is_type(qr_data, json_type_string)) {
                // Initialize QR format from JSON
                QRFormat format;
                init_default_qr_format_from_json(block, &format);
                
                // Calculate the QR code height approximately
                int qr_approx_width = 30 * format.size; // Based on scale
                current_y += qr_approx_width + 10; // Add some padding after QR
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

// Function to calculate the height required for the text
int calculate_text_height(const char *text, int width) {
    cairo_surface_t *dummy_surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, width, 100);
    cairo_t *cr = cairo_create(dummy_surface);
    
    PangoLayout *layout = pango_cairo_create_layout(cr);
    
    // Create font string with configurable weight and size
    char font_string[256];
    snprintf(font_string, sizeof(font_string), "%s %s %d", DEFAULT_FONT_NAME, DEFAULT_FONT_WEIGHT, DEFAULT_FONT_SIZE);
    PangoFontDescription *font_desc = pango_font_description_from_string(font_string);
    
    pango_layout_set_font_description(layout, font_desc);
    pango_layout_set_text(layout, text, -1);
    pango_layout_set_width(layout, width * PANGO_SCALE);
    pango_layout_set_wrap(layout, PANGO_WRAP_WORD_CHAR);
    
    int text_width, text_height;
    pango_layout_get_pixel_size(layout, &text_width, &text_height);
    
    // Cleanup
    pango_font_description_free(font_desc);
    g_object_unref(layout);
    cairo_destroy(cr);
    cairo_surface_destroy(dummy_surface);
    
    return text_height;
}

void render_text_to_surface(cairo_surface_t *surface, const char *text, int x, int y) {
    cairo_t *cr = cairo_create(surface);
    
    // Set up Pango for text rendering
    PangoLayout *layout = pango_cairo_create_layout(cr);
    
    // Create font string with configurable weight and size
    char font_string[256];
    snprintf(font_string, sizeof(font_string), "%s %s %d", DEFAULT_FONT_NAME, DEFAULT_FONT_WEIGHT, DEFAULT_FONT_SIZE);
    PangoFontDescription *font_desc = pango_font_description_from_string(font_string);
    
    pango_layout_set_font_description(layout, font_desc);
    pango_layout_set_text(layout, text, -1);
    
    // Calculate available width for text (full width minus margins)
    int text_width = PRINTER_WIDTH_DOTS - (2 * x); // Subtract left and right margins
    pango_layout_set_width(layout, text_width * PANGO_SCALE);
    pango_layout_set_wrap(layout, PANGO_WRAP_WORD_CHAR);
    
    // Set color to black
    cairo_set_source_rgb(cr, 0, 0, 0);
    
    // Move to position
    cairo_move_to(cr, x, y);
    
    // Render the layout
    pango_cairo_show_layout(cr, layout);
    
    // Cleanup
    pango_font_description_free(font_desc);
    g_object_unref(layout);
    cairo_destroy(cr);
}

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

unsigned char* convert_to_1bit_bmp(cairo_surface_t *surface, int *width, int *height, int *size) {
    *width = cairo_image_surface_get_width(surface);
    *height = cairo_image_surface_get_height(surface);
    
    unsigned char *data = cairo_image_surface_get_data(surface);
    int stride = cairo_image_surface_get_stride(surface);
    
    // Calculate number of bytes needed for 1-bit bitmap (8 pixels per byte)
    int bytes_per_row = (*width + 7) / 8;  // Round up to nearest byte
    *size = bytes_per_row * (*height);
    
    unsigned char *bitmap = malloc(*size);
    if (!bitmap) {
        printf("Error: Could not allocate memory for bitmap\n");
        return NULL;
    }
    
    // Convert ARGB32 to 1-bit bitmap (black and white)
    for (int y = 0; y < *height; y++) {
        for (int x = 0; x < *width; x++) {
            // Get pixel from ARGB32 surface (in BGRA format)
            int pixel_offset = y * stride + x * 4;  // 4 bytes per pixel
            
            // Extract RGB values (ARGB32 format)
            unsigned char b = data[pixel_offset];
            unsigned char g = data[pixel_offset + 1];
            unsigned char r = data[pixel_offset + 2];
            
            // Calculate grayscale value (luminance)
            int gray = (int)(0.299 * r + 0.587 * g + 0.114 * b);
            
            // Convert to 1-bit (white=1, black=0)
            int bit_pos = x % 8;
            int byte_idx = y * bytes_per_row + (x / 8);
            
            if (gray < 128) {  // Black threshold
                bitmap[byte_idx] |= (0x80 >> bit_pos);  // Set bit (MSB first format)
            } else {
                bitmap[byte_idx] &= ~(0x80 >> bit_pos);  // Clear bit
            }
        }
    }
    
    return bitmap;
}

unsigned char* create_escpos_raster_command(unsigned char *bitmap_data, int width, int height, int *cmd_size) {
    // Calculate bytes needed for ESC/POS command
    // Format: GS v 0 m xL xH yL yH [bitmap data]
    int bytes_per_row = (width + 7) / 8;
    int bitmap_size = bytes_per_row * height;
    
    // Command size includes the ESC/POS header + the bitmap data
    *cmd_size = 10 + bitmap_size;  // 6 bytes for header + 2 bytes each for x and y + bitmap data + 2 bytes for footer
    
    unsigned char *command = malloc(*cmd_size);
    if (!command) {
        printf("Error: Could not allocate memory for ESC/POS command\n");
        return NULL;
    }
    
    int idx = 0;
    
    // Initialize printer command
    command[idx++] = 0x1B;  // ESC
    command[idx++] = '@';   // @ (initialize)
    
    // GS v 0 command (raster bit image)
    command[idx++] = 0x1D;  // GS
    command[idx++] = 'v';   // v
    command[idx++] = '0';   // 0 (raster format)
    
    // Width and height (in bytes)
    unsigned char xL = bytes_per_row & 0xFF;      // Lower byte
    unsigned char xH = (bytes_per_row >> 8) & 0xFF;  // Higher byte
    unsigned char yL = height & 0xFF;             // Lower byte
    unsigned char yH = (height >> 8) & 0xFF;      // Higher byte
    
    command[idx++] = 0;      // m (mode - 0 for normal)
    command[idx++] = xL;
    command[idx++] = xH;
    command[idx++] = yL;
    command[idx++] = yH;
    
    // Copy bitmap data
    memcpy(&command[idx], bitmap_data, bitmap_size);
    idx += bitmap_size;
    
    // Add a feed and cut command at the end
    command[idx++] = 0x1B;  // ESC
    command[idx++] = 'd';   // d (feed n lines)
    command[idx++] = 3;     // Feed 3 lines
    
    command[idx++] = 0x1D;  // GS
    command[idx++] = 'V';   // V
    command[idx++] = 1;     // Full cut
    
    *cmd_size = idx;
    return command;
}

int send_to_usb_printer(unsigned char *data, int size, const char *device_path) {
    int fd = open(device_path, O_WRONLY);
    if (fd < 0) {
        perror("Failed to open printer device");
        return -1;
    }
    
    ssize_t bytes_written = write(fd, data, size);
    if (bytes_written != size) {
        perror("Failed to write all data to printer");
        close(fd);
        return -1;
    }
    
    // Force flush the data
    fsync(fd);
    
    close(fd);
    return 0; // Success
}

void save_to_file(unsigned char *data, int size) {
    FILE *fp = fopen("output.bin", "wb");
    if (fp) {
        fwrite(data, 1, size, fp);
        fclose(fp);
        printf("ESC/POS output written to output.bin\n");
    } else {
        printf("Could not write output.bin\n");
    }
}

void save_to_png(cairo_surface_t *surface, const char *filename) {
    cairo_status_t status = cairo_surface_write_to_png(surface, filename);
    if (status == CAIRO_STATUS_SUCCESS) {
        printf("PNG preview saved to %s\n", filename);
    } else {
        printf("Error saving PNG: %s\n", cairo_status_to_string(status));
    }
}

// Function to calculate the height required for both text and QR code
int calculate_combined_height(const char *text, int text_width, int qr_width) {
    int text_height = 0;
    int qr_height = 0;
    
    if (text && strlen(text) > 0) {
        text_height = calculate_text_height(text, text_width);
    }
    
    if (qr_width > 0) {
        qr_height = qr_width;  // Square QR code
    }
    
    return text_height + qr_height;
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
                
                // Render QR with format
                render_qr_to_surface_with_format(surface, qr_data_str, current_y, &format);
                
                // Calculate the QR code height approximately
                int qr_approx_width = 30 * format.size; // Based on scale
                current_y += qr_approx_width + 10; // Add some padding after QR
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