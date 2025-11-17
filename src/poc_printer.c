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

// Font configuration
#define DEFAULT_FONT_NAME "Noto Sans Malayalam"

// Printer configuration
#define PRINTER_WIDTH_MM 48  // Effective printing width from KP628E manual (48mm)
#define PRINTER_WIDTH_DOTS 384  // Standard thermal printer width in dots (48mm * 8 dots/mm = 384 dots)
#define DPI 203              // Dots per inch from KP628E manual (8 dots/mm = 203 DPI)

// ESC/POS Commands
#define ESC_CMD '@'          // Initialize printer
#define GS_CMD 'v'           // Raster bitmap command
#define GS_SUBCMD '0'        // Raster format 0
#define ESC_CMD_FEED 'd'     // Feed n lines
#define GS_CMD_CUT 'V'       // Paper cut

// Function declarations
void render_text_to_surface(cairo_surface_t *surface, const char *text, int x, int y);
unsigned char* convert_to_1bit_bmp(cairo_surface_t *surface, int *width, int *height, int *size);
unsigned char* create_escpos_raster_command(unsigned char *bitmap_data, int width, int height, int *cmd_size);
int send_to_usb_printer(unsigned char *data, int size, const char *device_path);
void save_to_file(unsigned char *data, int size);

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Usage: %s \"text to print\" [printer_device_path]\n", argv[0]);
        printf("Example: %s \"ഹലോ\" /dev/usb/lp0\n", argv[0]);
        return 1;
    }

    const char *text = argv[1];
    const char *printer_device = (argc > 2) ? argv[2] : NULL;  // Optional printer device path
    
    // Create a Cairo surface
    cairo_surface_t *surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, PRINTER_WIDTH_DOTS, 800); // Increased height to accommodate larger font
    cairo_t *cr = cairo_create(surface);
    
    // Fill background as white
    cairo_set_source_rgb(cr, 1, 1, 1);  // White background
    cairo_paint(cr);
    
    // Set font and render text
    render_text_to_surface(surface, text, 10, 20);
    
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
        printf("To send to printer directly: %s \"text\" /dev/usb/lp0\n", argv[0]);
    }
    
    // Cleanup
    cairo_destroy(cr);
    cairo_surface_destroy(surface);
    free(bmp_data);
    free(escpos_cmd);
    
    printf("Processed text: %s\n", text);
    return 0;
}

void render_text_to_surface(cairo_surface_t *surface, const char *text, int x, int y) {
    cairo_t *cr = cairo_create(surface);
    
    // Set up Pango for text rendering
    PangoLayout *layout = pango_cairo_create_layout(cr);
    PangoFontDescription *font_desc = pango_font_description_from_string(DEFAULT_FONT_NAME " Bold 32"); // Increased size to 32 and made bold
    
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