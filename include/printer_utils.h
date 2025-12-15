#ifndef PRINTER_UTILS_H
#define PRINTER_UTILS_H

#include <cairo.h>
#include <fcntl.h>
#include <json-c/json.h>
#include <linux/usbdevice_fs.h>
#include <pango/pango.h>
#include <pango/pangocairo.h>
#include <qrencode.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>

// Font configuration
#define DEFAULT_FONT_NAME "Noto Sans Malayalam"
#define DEFAULT_FONT_SIZE 24
#define DEFAULT_FONT_WEIGHT                                                    \
  "Light" // Change to "Bold", "Thin", "Light", etc. as needed

// Printer configuration
#define PRINTER_WIDTH_MM                                                       \
  48 // Effective printing width from KP628E manual (48mm)
#define PRINTER_WIDTH_DOTS                                                     \
  384 // Standard thermal printer width in dots (48mm * 8 dots/mm = 384 dots)
#define DPI 203 // Dots per inch from KP628E manual (8 dots/mm = 203 DPI)

// Padding configuration
#define TOP_PADDING 0    // Top padding in pixels
#define BOTTOM_PADDING 0 // Bottom padding in pixels
#define SIDE_MARGIN 10   // Side margin in pixels

// ESC/POS Commands
#define ESC_CMD '@'      // Initialize printer
#define GS_CMD 'v'       // Raster bitmap command
#define GS_SUBCMD '0'    // Raster format 0
#define ESC_CMD_FEED 'd' // Feed n lines
#define GS_CMD_CUT 'V'   // Paper cut
#define CUT_MODE_FULL 0
#define CUT_MODE_PARTIAL 1
#define CUT_MODE_NONE 2

// Structure to hold text formatting options
typedef struct {
  int font_size;
  char font_weight[32];
  char alignment[16]; // "left", "center", "right"
  int top_padding;
  int bottom_padding;
  int left_padding;
  int right_padding;
  float line_spacing; // 0.0 means default (usually 1.0)
} TextFormat;

// Structure to hold QR formatting options
typedef struct {
  int size;
  int x_pos; // 0=center, negative=left, positive=right margin
  int y_pos;
} QRFormat;

// Structure to hold image formatting options
typedef struct {
  int width;          // Desired width in pixels
  int height;         // Desired height in pixels
  int x_pos;          // 0=center, negative=left, positive=right margin
  int y_pos;          // Vertical position
  char filename[256]; // Image file path
  int scale;          // Scale factor (default 1)
} ImageFormat;

// Function declarations
int calculate_text_height(const char *text, int width);
void render_text_to_surface(cairo_surface_t *surface, const char *text, int x,
                            int y);
void render_text_to_surface_with_format(cairo_surface_t *surface,
                                        const char *text, int base_x,
                                        int base_y, TextFormat *format);
void render_qr_to_surface(cairo_surface_t *surface, const char *qr_data, int x,
                          int y, int scale);
void render_qr_to_surface_with_format(cairo_surface_t *surface,
                                      const char *qr_data, int base_y,
                                      QRFormat *format);
void render_image_to_surface_with_format(cairo_surface_t *surface, int base_y,
                                         ImageFormat *format);
unsigned char *convert_to_1bit_bmp(cairo_surface_t *surface, int *width,
                                   int *height, int *size);
unsigned char *create_escpos_raster_command(unsigned char *bitmap_data,
                                            int width, int height, int cut_mode,
                                            int *cmd_size);
int send_to_usb_printer(unsigned char *data, int size, const char *device_path);
void save_to_file(unsigned char *data, int size);
void save_to_png(cairo_surface_t *surface, const char *filename);
int calculate_combined_height(const char *text, int text_width, int qr_width);
void render_json_blocks(cairo_surface_t *surface, const char *json_str);
int calculate_total_height(const char *json_str);
void init_default_text_format(TextFormat *format);
void init_default_text_format_from_json(json_object *block, TextFormat *format);
void init_default_qr_format(QRFormat *format);
void init_default_qr_format_from_json(json_object *block, QRFormat *format);
void init_default_image_format_from_json(json_object *block,
                                         ImageFormat *format);
int calculate_text_height_with_format(const char *text, int width,
                                      TextFormat *format);
cairo_surface_t *create_qr_surface(const char *data, int scale);
cairo_surface_t *create_image_surface(const char *filename, int target_width,
                                      int target_height);
void set_default_font_size(int size);
int get_default_font_size();

#endif // PRINTER_UTILS_H